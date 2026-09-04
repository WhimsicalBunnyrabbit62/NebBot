import javax.swing.*;
import java.awt.*;
import java.awt.event.ActionEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;

public class CreateBoard extends JPanel {
    private int totalTurns = 1;
    private int turnsSinceCapture = 0;
    private final java.util.HashMap<String, Integer> positionCounts = new java.util.HashMap<>();
    private boolean gameOver = false;
    private int TILESIZE = 80;
    // Numerated Turns
    private static final int WHITE = 1;
    private static final int BLACK = -1;
    // Move Flags
    private static final int MOVE_NONE = 0;
    private static final int EN_PASSANT = 1;
    private static final int CASTLE_KING = 2;
    private static final int CASTLE_QUEEN = 3;
    private static final int PROMOTION_QUEEN = 4;
    private static final int PROMOTION_ROOK = 5;
    private static final int PROMOTION_BISHOP = 6;
    private static final int PROMOTION_KNIGHT = 7;
    private static final int DOUBLE_PAWN_PUSH = 8;
    private int selectedTile = -1;
    private int sourceTile = -1;
    private int lastMoveFrom = -1;
    private int lastMoveTo = -1;
    private int currentTurn = 1; // 1 white : -1 black
    private boolean playerStarting = true; // change to change colors
    private boolean canMakeEngineMove = !playerStarting;
    private boolean canCastleWhiteKing = true;
    private boolean canCastleBlackKing = true;
    private boolean canCastleWhiteQueen = true;
    private boolean canCastleBlackQueen = true;
    private boolean castling = false;
    private int enPassantTarget = -1;
    private EngineBridge bridge = new EngineBridge("./chess_engine");
    private boolean firstListener = true;

    int flags = 0;

    private final int[] board = {
        12, 10, 11, 13, 14, 11, 10, 12, 
        9,  9,  9,  9,  9,  9,  9,  9,   // black back rank
        0,  0,  0,  0,  0,  0,  0,  0,   // black pawns
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,
        1,  1,  1,  1,  1,  1,  1,  1,   // white pawns
        4,  2,  3,  5,  6,  3,  2,  4    // white back rank
    };

    private Image[] pieceImages = new Image[15];

    public CreateBoard() {
        setBackground(new Color(38, 36, 33));
        loadImages();

        bridge.startListening(firstListener);
        firstListener = false;
        bridge.sendCommand("uci");
        bridge.sendCommand("isready");
        bridge.sendCommand("position fen " + getFen());
        positionCounts.merge(positionKey(), 1, Integer::sum); // count the starting position
        // bridge.sendCommand("perft 6");

        addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                if (gameOver) return;
                int boardOriginX = getBoardOriginX();
                int boardOriginY = getBoardOriginY();
                int col = (e.getX() - boardOriginX) / TILESIZE;
                int row = (e.getY() - boardOriginY) / TILESIZE;

                int playerMove = (playerStarting) ? 1 : -1;
                int engineMove = playerMove * -1;
                if (currentTurn == playerMove) {
                    if (col >= 0 && col < 8 && row >= 0 && row < 8) {
                    int clickedIndex = row * 8 + col;
                    if (!playerStarting) clickedIndex = 63 - clickedIndex;
                    
                        if (sourceTile == -1) {
                            if (board[clickedIndex] != 0) {
                                boolean isWhitePiece = board[clickedIndex] < 7;
                                boolean isWhiteTurn = (currentTurn == 1);

                                if (isWhitePiece == isWhiteTurn) {
                                    sourceTile = clickedIndex;
                                } else {
                                    System.out.println("its not your turn. buddy.");
                                }
                            } 
                        } else {
                            if (clickedIndex == sourceTile) {
                                sourceTile = -1;
                            } else {
                                if (moveLegal(sourceTile, clickedIndex)) {
                                    int piece = board[sourceTile];
                                    int myColor = currentTurn;
                                    int enemyColor = (myColor == WHITE) ? BLACK : WHITE;
                                    int capturedPiece = board[clickedIndex];
                                    int oldEP = enPassantTarget;
                                    boolean oldCWK = canCastleWhiteKing;
                                    boolean oldCBK = canCastleBlackKing;
                                    boolean oldCWQ = canCastleWhiteQueen;
                                    boolean oldCBQ = canCastleBlackQueen;
                                    int flags = computeMoveFlags(sourceTile, clickedIndex, board[sourceTile], capturedPiece);

                                    if (capturedPiece == 0 && piece != 9 && piece != 1) turnsSinceCapture++;
                                    else turnsSinceCapture = 0;
                                    
                                    if (piece == 6 && castling == false) {
                                        canCastleWhiteKing = false;
                                        canCastleWhiteQueen = false;
                                    }
                                    if (piece == 14 && castling == false) {
                                        canCastleBlackKing = false;
                                        canCastleBlackQueen = false;
                                    }

                                    makeHumanMove(sourceTile, clickedIndex);
                                    if (flags >= PROMOTION_QUEEN && flags <= PROMOTION_KNIGHT) {
                                        flags = promotionFlagFromPiece(board[clickedIndex]);
                                    }

                                    int kingSq = findKing(myColor);
                                    if (kingSq != -1 && isSquareAttacked(kingSq, enemyColor)) {
                                        unmakeHumanMove(sourceTile, clickedIndex, flags, capturedPiece, oldEP, oldCWK, oldCBK, oldCWQ, oldCBQ);
                                        System.out.println("ya move wong");
                                        castling = false;
                                        sourceTile = -1;
                                        repaint();
                                        return;
                                    }

                                    lastMoveFrom = sourceTile;
                                    lastMoveTo = clickedIndex;

                                    boolean irreversible = (capturedPiece != 0 || piece == 1 || piece == 9);
                                    recordPositionAndCheckDraw(irreversible);
                                } else {
                                    System.out.println("ya move wong");
                                }
                                
                                
                                castling = false;
                                sourceTile = -1;
                                repaint();
                            }
                        }
                    }

                    canMakeEngineMove = true;
                } 
                if (currentTurn == engineMove && canMakeEngineMove) {
                    canMakeEngineMove = false;
                    runEngineTurn();
                    totalTurns++;
                }  

                repaint();
            }
        });

        // L to load a fen
        setFocusable(true);
        getInputMap(WHEN_IN_FOCUSED_WINDOW).put(KeyStroke.getKeyStroke('l'), "loadFen");
        getInputMap(WHEN_IN_FOCUSED_WINDOW).put(KeyStroke.getKeyStroke('L'), "loadFen");
        getActionMap().put("loadFen", new AbstractAction() {
            @Override   
            public void actionPerformed(ActionEvent e) {
                promptForFen();
            }
        });
        System.out.println("Tip: press \"L\" to load a FEN and play from that position.");
    }

    // Ask the user for a FEN, load it, and let the engine reply if it is its turn.
    private void promptForFen() {
        String fen = JOptionPane.showInputDialog(this,
                "Paste a FEN to load, then play from that position:",
                "Load FEN", JOptionPane.PLAIN_MESSAGE);
        if (fen == null) return; // cancelled

        if (loadFromFen(fen)) {
            int engineColor = playerStarting ? BLACK : WHITE;
            if (currentTurn == engineColor) {
                canMakeEngineMove = false;
                runEngineTurn();
            } else {
                canMakeEngineMove = true;
            }
        }
    }

    public boolean loadFromFen(String fen) {
        if (fen == null) return false;
        fen = fen.trim();
        if (fen.isEmpty()) return false;

        String[] parts = fen.split("\\s+");
        String[] rows = parts[0].split("/");
        if (rows.length != 8) {
            showFenError("A FEN must have 8 ranks separated by '/'.");
            return false;
        }

        int[] temp = new int[64];
        for (int r = 0; r < 8; r++) {
            int col = 0;
            for (char c : rows[r].toCharArray()) {
                if (Character.isDigit(c)) {
                    col += c - '0';
                } else {
                    int code = charToPieceCode(c);
                    if (code == 0) {
                        showFenError("Unexpected character '" + c + "' in the piece layout.");
                        return false;
                    }
                    if (col > 7) {
                        showFenError("Rank " + (8 - r) + " has too many squares.");
                        return false;
                    }
                    temp[r * 8 + col] = code;
                    col++;
                }
            }
            if (col != 8) {
                showFenError("Rank " + (8 - r) + " does not describe 8 squares.");
                return false;
            }
        }

        // Side to move 
        int newTurn = (parts.length >= 2 && parts[1].equalsIgnoreCase("b")) ? BLACK : WHITE;

        // Castling rights
        boolean wk = false, wq = false, bk = false, bq = false;
        if (parts.length >= 3 && !parts[2].equals("-")) {
            wk = parts[2].contains("K");
            wq = parts[2].contains("Q");
            bk = parts[2].contains("k");
            bq = parts[2].contains("q");
        }

        // En passant target square.
        int ep = -1;
        if (parts.length >= 4 && !parts[3].equals("-") && parts[3].length() == 2) {
            ep = algebraicToIndex(parts[3]);
        }

        int halfmove = 0, fullmove = 1;
        if (parts.length >= 5) {
            try { halfmove = Integer.parseInt(parts[4]); } catch (NumberFormatException ignored) {}
        }
        if (parts.length >= 6) {
            try { fullmove = Integer.parseInt(parts[5]); } catch (NumberFormatException ignored) {}
        }

        System.arraycopy(temp, 0, board, 0, 64);
        currentTurn = newTurn;
        canCastleWhiteKing = wk;
        canCastleWhiteQueen = wq;
        canCastleBlackKing = bk;
        canCastleBlackQueen = bq;
        enPassantTarget = ep;
        turnsSinceCapture = halfmove;
        totalTurns = fullmove;
        sourceTile = -1;
        castling = false;
        lastMoveFrom = -1;
        lastMoveTo = -1;

        // New starting position: reset repetition tracking and seed with this position.
        gameOver = false;
        positionCounts.clear();
        positionCounts.merge(positionKey(), 1, Integer::sum);

        bridge.sendCommand("position fen " + getFen());
        repaint();
        return true;
    }

    private void showFenError(String detail) {
        JOptionPane.showMessageDialog(this, "Invalid FEN: " + detail,
                "Load FEN", JOptionPane.ERROR_MESSAGE);
    }

    private int charToPieceCode(char c) {
        switch (c) {
            case 'P': return 1;  case 'N': return 2;  case 'B': return 3;
            case 'R': return 4;  case 'Q': return 5;  case 'K': return 6;
            case 'p': return 9;  case 'n': return 10; case 'b': return 11;
            case 'r': return 12; case 'q': return 13; case 'k': return 14;
            default:  return 0;
        }
    }

    public void makeHumanMove(int sourceTile, int clickedIndex) {
        int piece = board[sourceTile];
        int capturedPiece = board[clickedIndex];
        boolean isPromotionMove = (piece == 1 && clickedIndex < 8) || (piece == 9 && clickedIndex >= 56);

        if ((board[sourceTile] == 1 || board[sourceTile] == 9) && clickedIndex == enPassantTarget) {
            int victimIndex = (currentTurn == 1) ? clickedIndex + 8 : clickedIndex - 8;
            board[victimIndex] = 0; 
        }

        if (piece == 6) { // white king
            canCastleWhiteKing = false;
            canCastleWhiteQueen = false;
        } else if (piece == 14) { // black king
            canCastleBlackKing = false;
            canCastleBlackQueen = false;
        } else if (piece == 4 && sourceTile == 63) { // white king side rook
            canCastleWhiteKing = false;
        } else if (piece == 4 && sourceTile == 56) { // white queen side rook
            canCastleWhiteQueen = false;
        } else if (piece == 12 && sourceTile == 7) { // black king side rook
            canCastleBlackKing = false;
        } else if (piece == 12 && sourceTile == 0) { // black queen side rook
            canCastleBlackQueen = false;
        }

        // no castle rook captured
        if (capturedPiece == 4) {
            if (clickedIndex == 63) canCastleWhiteKing = false;
            if (clickedIndex == 56) canCastleWhiteQueen = false;
        } else if (capturedPiece == 12) {
            if (clickedIndex == 7) canCastleBlackKing = false;
            if (clickedIndex == 0) canCastleBlackQueen = false;
        }

        // castling
        if (castling) {
            if (clickedIndex == 58 || clickedIndex == 2) {
                castleQueen(currentTurn == 1);
            } else {
                castleKing(currentTurn == 1);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
            }

            if (currentTurn == 1) {
                canCastleWhiteKing = false;
                canCastleWhiteQueen = false;
            } else {
                canCastleBlackKing = false;
                canCastleBlackQueen = false;
            }
        } else {
            board[clickedIndex] = board[sourceTile];
            board[sourceTile] = 0;
        }

        // promotion
        if (isPromotionMove) {
            int promoteTo = choosePromotionPiece(currentTurn);
            board[clickedIndex] = promoteTo;
        }

        // EP target
        if (piece == 1 && sourceTile - clickedIndex == 16) {
            enPassantTarget = sourceTile - 8;
        } else if (piece == 9 && clickedIndex - sourceTile == 16) {
            enPassantTarget = sourceTile + 8;
        } else {
            enPassantTarget = -1;
        }

        currentTurn = (currentTurn == 1) ? -1 : 1;
        castling = false;
    }

    public void unmakeHumanMove(int from, int to, int flags, int capturedPiece, int oldEP, boolean oldCanCastleWhiteKing, boolean oldCanCastleBlackKing, boolean oldCanCastleWhiteQueen, boolean oldCanCastleBlackQueen) {
        currentTurn = (currentTurn == 1) ? -1 : 1;

        if (flags >= PROMOTION_QUEEN && flags <= PROMOTION_KNIGHT) {
            board[from] = (currentTurn == WHITE) ? 1 : 9;
        } else {
            board[from] = board[to];
        }

        board[to] = capturedPiece;

        if (flags == EN_PASSANT) {
            board[to] = 0;
            int victimSq = (currentTurn == WHITE) ? to + 8 : to - 8;
            board[victimSq] = (currentTurn == WHITE) ? 9 : 1;
        } else if (flags == CASTLE_KING) {
            if (currentTurn == WHITE) {
                board[63] = 4;
                board[61] = 0;
            } else {
                board[7] = 12;
                board[5] = 0;
            }
        } else if (flags == CASTLE_QUEEN) {
            if (currentTurn == WHITE) {
                board[56] = 4;
                board[59] = 0;
            } else {
                board[0] = 12;
                board[3] = 0;
            }
        }

        enPassantTarget = oldEP;
        canCastleWhiteKing = oldCanCastleWhiteKing;
        canCastleBlackKing = oldCanCastleBlackKing;
        canCastleWhiteQueen = oldCanCastleWhiteQueen;
        canCastleBlackQueen = oldCanCastleBlackQueen;
    }

    private int choosePromotionPiece(int color) {
        String[] options = {"Queen", "Rook", "Bishop", "Knight"};
        int choice = JOptionPane.showOptionDialog(
                this,
                "Promote pawn to:",
                "Promotion",
                JOptionPane.DEFAULT_OPTION,
                JOptionPane.QUESTION_MESSAGE,
                null,
                options,
                options[0]
        );

        if (choice < 0) choice = 0; 

        if (color == WHITE) {
            switch (choice) {
                case 1: return 4; // Rook
                case 2: return 3; // Bishop
                case 3: return 2; // Knight
                default: return 5; // Queen
            }
        } else {
            switch (choice) {
                case 1: return 12; // Rook
                case 2: return 11; // Bishop
                case 3: return 10; // Knight
                default: return 13; // Queen
            }
        }
    }

    private void runEngineTurn() {
        if (gameOver) return;
        bridge.sendCommand("position fen " + getFen());
        bridge.sendCommand("go");
        new Thread(() -> {
            String bestMove = bridge.waitForBestMove();
            System.out.println("GOT: [" + bestMove + "]");

            if (bestMove == null || bestMove.contains("checkmate") || bestMove.contains("stalemate")) {
                System.out.println(bestMove);
                return;
            }

            SwingUtilities.invokeLater(() -> makeEngineMove(bestMove));
        }).start();
    }

    private void loadImages() {
        for (int i = 1; i <= 14; i++) {
            if (i == 7 || i == 8) continue;

            String path = "res/" + i + ".png";
            File file = new File(path);
            
            if (file.exists()) {
                pieceImages[i] = new ImageIcon(path).getImage();
            } else {
                System.out.println("someting wong");
            }
        }
    }

    private static final int BAR_H = 48;

    private int getBoardSize() {
        return Math.min(getWidth(), getHeight() - 2 * BAR_H) - 20;
    }

    private int getBoardOriginX() {
        return (getWidth() - getBoardSize()) / 2;
    }

    private int getBoardOriginY() {
        return BAR_H + (getHeight() - 2 * BAR_H - getBoardSize()) / 2;
    }

    private int piecePointValue(int piece) {
        switch (piece) {
            case 1: case 9:  return 1; 
            case 2: case 10: return 3; 
            case 3: case 11: return 3; 
            case 4: case 12: return 5; 
            case 5: case 13: return 9; 
            default:         return 0; 
        }
    }

    private int materialFor(boolean white) {
        int sum = 0;
        for (int piece : board) {
            boolean isWhite = piece >= 1 && piece <= 6;
            boolean isBlack = piece >= 9 && piece <= 14;
            if (white ? isWhite : isBlack) sum += piecePointValue(piece);
        }
        return sum;
    }

    private int startingCount(int piece) {
        switch (piece) {
            case 1: case 9:  return 8; 
            case 2: case 10: return 2; 
            case 3: case 11: return 2; 
            case 4: case 12: return 2; 
            case 5: case 13: return 1; 
            default:         return 0;
        }
    }

    private int currentCount(int piece) {
        int c = 0;
        for (int p : board) if (p == piece) c++;
        return c;
    }

    private int capturedCount(int piece) {
        return Math.max(0, startingCount(piece) - currentCount(piece));
    }

    private static final Color LIGHT_SQUARE   = new Color(240, 217, 181);
    private static final Color DARK_SQUARE    = new Color(181, 136, 99);
    private static final Color LAST_MOVE_TINT = new Color(246, 217, 99, 150);
    private static final Color SELECT_TINT    = new Color(120, 190, 120, 160);
    private static final Color BOARD_FRAME     = new Color(60, 47, 33);

    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        Graphics2D g2 = (Graphics2D) g;
        g2.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
        g2.setRenderingHint(RenderingHints.KEY_INTERPOLATION, RenderingHints.VALUE_INTERPOLATION_BILINEAR);
        g2.setRenderingHint(RenderingHints.KEY_RENDERING, RenderingHints.VALUE_RENDER_QUALITY);
        g2.setRenderingHint(RenderingHints.KEY_TEXT_ANTIALIASING, RenderingHints.VALUE_TEXT_ANTIALIAS_ON);

        TILESIZE = getBoardSize() / 8;
        int boardOriginX = getBoardOriginX();
        int boardOriginY = getBoardOriginY();
        int boardSize = TILESIZE * 8;

        g2.setColor(new Color(0, 0, 0, 45));
        g2.fillRoundRect(boardOriginX - 3, boardOriginY - 3 + 4, boardSize + 6, boardSize + 6, 14, 14);
        g2.setColor(BOARD_FRAME);
        g2.fillRoundRect(boardOriginX - 3, boardOriginY - 3, boardSize + 6, boardSize + 6, 14, 14);

        int labelFontSize = Math.max(10, TILESIZE / 7);
        Font labelFont = new Font("SansSerif", Font.BOLD, labelFontSize);

        for (int i = 0; i < 64; i++) {
            int displayIndex = playerStarting ? i : 63 - i;
            int row = displayIndex / 8;
            int col = displayIndex % 8;
            int x = boardOriginX + col * TILESIZE;
            int y = boardOriginY + row * TILESIZE;
            boolean lightSquare = (row + col) % 2 == 0;

            g2.setColor(lightSquare ? LIGHT_SQUARE : DARK_SQUARE);
            g2.fillRect(x, y, TILESIZE, TILESIZE);

            if (i == lastMoveFrom || i == lastMoveTo) {
                g2.setColor(LAST_MOVE_TINT);
                g2.fillRect(x, y, TILESIZE, TILESIZE);
            }

            if (i == sourceTile) {
                g2.setColor(SELECT_TINT);
                g2.fillRect(x, y, TILESIZE, TILESIZE);
            }

            Color labelColor = lightSquare ? DARK_SQUARE : LIGHT_SQUARE;
            g2.setFont(labelFont);
            g2.setColor(labelColor);
            if (row == 7) {
                String fileLabel = String.valueOf((char) ('a' + (i % 8)));
                FontMetrics fm = g2.getFontMetrics();
                g2.drawString(fileLabel, x + TILESIZE - fm.stringWidth(fileLabel) - 4, y + TILESIZE - 5);
            }
            if (col == 0) {
                String rankLabel = String.valueOf(8 - (i / 8));
                g2.drawString(rankLabel, x + 4, y + g2.getFontMetrics().getAscent() + 2);
            }

            int pieceValue = board[i];
            if (pieceValue != 0 && pieceImages[pieceValue] != null) {
                int pad = Math.max(2, TILESIZE / 22);
                g2.drawImage(pieceImages[pieceValue], x + pad, y + pad, TILESIZE - 2 * pad, TILESIZE - 2 * pad, null);
            }
        }

        int diff = materialFor(true) - materialFor(false);
        boolean bottomIsWhite = playerStarting;
        int topBarCenterY = boardOriginY - BAR_H / 2;
        int bottomBarCenterY = boardOriginY + boardSize + BAR_H / 2;

        drawPlayerBar(g2, boardOriginX, topBarCenterY,
                !bottomIsWhite, !bottomIsWhite ? diff : -diff);

        drawPlayerBar(g2, boardOriginX, bottomBarCenterY,
                bottomIsWhite, bottomIsWhite ? diff : -diff);
    }

    private void drawPlayerBar(Graphics2D g2, int boardOriginX, int barCenterY,
                               boolean barIsWhite, int advantage) {
        int icon = 26;
        int x = boardOriginX + 4;
        int y = barCenterY - icon / 2;

        int[] order = barIsWhite ? new int[]{9, 10, 11, 12, 13}   
                                  : new int[]{1, 2, 3, 4, 5};    
        for (int type : order) {
            int cnt = capturedCount(type);
            for (int k = 0; k < cnt; k++) {
                if (pieceImages[type] != null) {
                    g2.drawImage(pieceImages[type], x, y, icon, icon, null);
                }
                x += (int) (icon * 0.5); 
            }
            if (cnt > 0) x += (int) (icon * 0.3); 
        }

        if (advantage > 0) {
            g2.setFont(new Font("SansSerif", Font.BOLD, 16));
            FontMetrics fm = g2.getFontMetrics();
            g2.setColor(new Color(255, 255, 255));
            g2.drawString("+" + advantage, x + 6, barCenterY + fm.getAscent() / 2 - 1);
        }
    }

    public boolean moveLegal(int start, int end) {
        int piece = board[start];

        if (board[end] != 0) {
            boolean startIsWhite = board[start] < 7;
            boolean endIsWhite = board[end] < 7;
            if (startIsWhite == endIsWhite) return false; 
        }

        if (piece == 1) { // White Pawn check
            int startCol = start % 8;
            int endCol = end % 8;
            int colDiff = Math.abs(startCol - endCol);

            if (startCol == endCol && board[end] == 0) {
                if (start - end == 8) return true;
                if (start - end == 16 && start / 8 == 6) return isPathClear(start, end);
            }

            if (start - end == 7 || start - end == 9) {
                if (colDiff == 1) {
                    if (board[end] != 0) return true;
                    
                    if (end == enPassantTarget) {
                        int victim = end + 8;
                        if (victim >= 0 && victim < 64 && board[victim] == 9) return true;
                    }
                }
            }

            return false;
        }

        if (piece == 9) { // Black Pawn check
            int startCol = start % 8;
            int endCol = end % 8;
            int colDiff = Math.abs(startCol - endCol);

            if (startCol == endCol && board[end] == 0) {
                if (end - start == 8) return true;
                if (end - start == 16 && start / 8 == 1) return isPathClear(start, end);
            }

            if (end - start == 7 || end - start == 9) {
                if (colDiff == 1) {
                    if (board[end] != 0) return true;
                    
                    if (end == enPassantTarget) {
                        int victim = end - 8;
                        if (victim >= 0 && victim < 64 && board[victim] == 1) return true;
                    }
                }
            }

            return false;
        }

        if (piece == 2 || piece == 10) { // Knight Check
            int startCol = start % 8;
            int startRow = start / 8;
            int endCol = end % 8;
            int endRow = end / 8;

            int colDiff = Math.abs(startCol - endCol);
            int rowDiff = Math.abs(startRow - endRow);

            return (colDiff == 1 && rowDiff == 2) || (colDiff == 2 && rowDiff == 1);
        } 

        if (piece == 3 || piece == 11) { // Bishop check
            int colDiff = Math.abs((start % 8) - (end % 8));
            int rowDiff = Math.abs((start / 8) - (end / 8));

            if (colDiff != rowDiff) {
                return false;
            }
            return isPathClear(start, end);
        }

        if (piece == 4 || piece == 12) { // Rook check
            return ((start / 8 == end / 8) || (start % 8 == end % 8)) && isPathClear(start, end);
        }

        if (piece == 5 || piece == 13) { // Queen check
            int colDiff = Math.abs((start % 8) - (end % 8));
            int rowDiff = Math.abs((start / 8) - (end / 8));


            return ((start / 8 == end / 8) || (start % 8 == end % 8) || colDiff == rowDiff) && isPathClear(start, end);
        }

        if (piece == 6 || piece == 14) { // King check
            int colDiff = Math.abs((start % 8) - (end % 8));
            int rowDiff = Math.abs((start / 8) - (end / 8));

            boolean kingCastle = (currentTurn == 1) ? canCastleWhiteKing : canCastleBlackKing;
            boolean queenCastle = (currentTurn == 1) ? canCastleWhiteQueen : canCastleBlackQueen;

            if (piece == 6 && kingCastle && start == 60) { // king
                if (end == 62 && board[61] == 0 && board[62] == 0) { 
                    castling = true;
                    canCastleWhiteKing = false; 
                    canCastleWhiteQueen = false;
                    return true;
                }
            }
            
            if (piece == 6 && queenCastle && start == 60) { // queen
                if (end == 58 && board[59] == 0 && board[58] == 0 && board[57] == 0) { 
                    castling = true;
                    canCastleWhiteQueen = false;
                    canCastleWhiteKing = false;
                    return true;
                }
            }

            if (piece == 14 && kingCastle && start == 4) { // king
                if (end == 6 && board[5] == 0 && board[6] == 0) { 
                    castling = true;
                    canCastleBlackKing = false; 
                    canCastleBlackQueen = false;
                    return true;
                }
            }
            
            if (piece == 14 && queenCastle && start == 4) { // queen
                if (end == 2 && board[1] == 0 && board[2] == 0 && board[3] == 0) { 
                    castling = true;
                    canCastleBlackQueen = false;
                    canCastleBlackKing = false;
                    return true;
                }
            }

            return colDiff <= 1 && rowDiff <= 1;
        }

        return true;
    }

    private boolean isSquareAttacked(int targetSq, int attackerColor) {
        // knight check
        int[] knightOffsets = {-17, -15, -10, -6, 6, 10, 15, 17};
        int enemyKnight = (attackerColor == WHITE) ? 2 : 10;
        for (int offset : knightOffsets) {
            int sq = targetSq + offset;
            if (sq >= 0 && sq < 64 && isSafeJump(sq, targetSq)) {
                if (board[sq] == enemyKnight) return true;
            }
        }

        // sliding check
        if (attackedBySlider(targetSq, attackerColor, new int[]{-8, 8, -1, 1}, true)) return true;
        if (attackedBySlider(targetSq, attackerColor, new int[]{-9, -7, 7, 9}, false)) return true;

        // pawn check
        int enemyPawn = (attackerColor == WHITE) ? 1 : 9;
        int[] pawnOffsets = (attackerColor == WHITE) ? new int[]{7, 9} : new int[]{-7, -9};
        for (int offset : pawnOffsets) {
            int sq = targetSq + offset;
            if (sq >= 0 && sq < 64 && Math.abs((sq % 8) - (targetSq % 8)) == 1) {
                if (board[sq] == enemyPawn) return true;
            }
        }

        // king check
        int enemyKing = (attackerColor == WHITE) ? 6 : 14;
        int[] kingOffsets = {-9, -8, -7, -1, 1, 7, 8, 9};
        for (int offset : kingOffsets) {
            int sq = targetSq + offset;
            if (sq >= 0 && sq < 64 && Math.abs((sq % 8) - (targetSq % 8)) <= 1) {
                if (board[sq] == enemyKing) return true;
            }
        }

        return false;
    }

    private boolean canMoveInDirection(int sq, int offset) {
        int file = sq % 8;
        if (file == 7 && (offset == 1 || offset == -7 || offset == 9)) return false;
        if (file == 0 && (offset == -1 || offset == 7 || offset == -9)) return false;
        return true;
    }

    private boolean isSafeJump(int startSq, int targetSq) {
        int startFile = startSq % 8;
        int startRank = startSq / 8;
        int endFile = targetSq % 8;
        int endRank = targetSq / 8;

        int df = Math.abs(startFile - endFile);
        int dr = Math.abs(startRank - endRank);
        return (df == 1 && dr == 2) || (df == 2 && dr == 1);
    }

    private boolean attackedBySlider(int targetSq, int attackerColor, int[] offsets, boolean isRook) {
        for (int offset : offsets) {
            int currentSq = targetSq;
            while (true) {
                if (!canMoveInDirection(currentSq, offset)) break;
                currentSq += offset;
                if (currentSq < 0 || currentSq >= 64) break;

                int piece = board[currentSq];
                if (piece != 0) {
                    boolean isEnemyPiece = (attackerColor == WHITE && piece < 7) || (attackerColor == BLACK && piece > 8);
                    if (isEnemyPiece) {
                        if (isRook) {
                            if (piece == 4 || piece == 12 || piece == 5 || piece == 13) return true;
                        } else {
                            if (piece == 3 || piece == 11 || piece == 5 || piece == 13) return true;
                        }
                    }
                    break;
                }
            }
        }
        return false;
    }

    private int findKing(int color) {
        int targetKing = (color == WHITE) ? 6 : 14;
        for (int i = 0; i < 64; i++) {
            if (board[i] == targetKing) return i;
        }
        return -1;
    }

    private int computeMoveFlags(int from, int to, int piece, int capturedPiece) {
        if ((piece == 1 || piece == 9) && to == enPassantTarget && capturedPiece == 0) {
            return EN_PASSANT;
        }
        if ((piece == 1 && from - to == 16) || (piece == 9 && to - from == 16)) {
            return DOUBLE_PAWN_PUSH;
        }
        if (piece == 6) {
            if (to == 62 && canCastleWhiteKing) return CASTLE_KING;
            if (to == 58 && canCastleWhiteQueen) return CASTLE_QUEEN;
        } else if (piece == 14) {
            if (to == 6 && canCastleBlackKing) return CASTLE_KING;
            if (to == 2 && canCastleBlackQueen) return CASTLE_QUEEN;
        }
        if ((piece == 1 && to < 8) || (piece == 9 && to >= 56)) {
            return PROMOTION_QUEEN;
        }
        return MOVE_NONE;
    }

    private int promotionFlagFromPiece(int piece) {
        switch (piece) {
            case 5: return PROMOTION_QUEEN;
            case 4: return PROMOTION_ROOK;
            case 3: return PROMOTION_BISHOP;
            case 2: return PROMOTION_KNIGHT;
            case 13: return PROMOTION_QUEEN;
            case 12: return PROMOTION_ROOK;
            case 11: return PROMOTION_BISHOP;
            case 10: return PROMOTION_KNIGHT;
            default: return PROMOTION_QUEEN;
        }
    }

    public void castleKing(boolean white) {
        if (white) {
            board[63] = 0;
            board[62] = 6;
            board[61] = 4;
            board[60] = 0;
        } else {
            board[4] = 0;
            board[7] = 0;
            board[6] = 14;
            board[5] = 12;
        }
    }

    public void castleQueen(boolean white) {
        if (white) {
            board[56] = 0;  
            board[57] = 0;  
            board[58] = 6;  
            board[59] = 4;  
            board[60] = 0;  
        } else {
            board[0] = 0;
            board[1] = 0;
            board[2] = 14;  
            board[3] = 12; 
            board[4] = 0;   
        }
    }

    public boolean isPathClear(int start, int end) {
        int diff = end - start;
        int step = 0;

        if (Math.abs(diff) % 8 == 0) step = (diff > 0) ? 8 : -8; 
        else if (start / 8 == end / 8) step = (diff > 0) ? 1 : -1; 
        else if (Math.abs(diff) % 7 == 0) step = (diff > 0) ? 7 : -7; 
        else if (Math.abs(diff) % 9 == 0) step = (diff > 0) ? 9 : -9;
        else return true;

        int current = start + step;
        while (current != end) {
            if (board[current] != 0) return false;
            current += step;
        }

        return true;
    }

    private String positionKey() {
        String[] parts = getFen().split(" ");
        return parts[0] + " " + parts[1] + " " + parts[2] + " " + parts[3];
    }

    private void recordPositionAndCheckDraw(boolean irreversible) {
        if (irreversible) positionCounts.clear();

        String key = positionKey();
        int count = positionCounts.merge(key, 1, Integer::sum);

        if (count >= 3) {
            announceDraw("Draw by threefold repetition");
        } else if (turnsSinceCapture >= 100) {
            announceDraw("Draw by fifty-move rule");
        }
    }

    private void announceDraw(String message) {
        if (gameOver) return;
        gameOver = true;
        repaint();
        SwingUtilities.invokeLater(() ->
            JOptionPane.showMessageDialog(null, "Game Over: " + message));
    }

    public String getFen() {
        StringBuilder fen = new StringBuilder();

        // inscribe the board into fen
        for (int row = 0; row < 8; row++) {
            int emptyCount = 0;

            for (int col = 0; col < 8; col++) {
                int piece = board[row * 8 + col];
                if (piece == 0) {
                    emptyCount++;
                } else {
                    if (emptyCount > 0) {
                        fen.append(emptyCount);
                        emptyCount = 0;
                    }
                    fen.append(getCharFromPiece(piece));
                }
            }

            if (emptyCount > 0) fen.append(emptyCount);
            if (row < 7) fen.append("/");
        }

        fen.append(" " + (currentTurn == 1 ? "w" : "b") + " "); // fen move

        // fen castling rights
        boolean hasCastle = false;
        if (canCastleWhiteKing) {
            fen.append("K");
            hasCastle = true;
        } 
        if (canCastleWhiteQueen) {
            fen.append("Q");
            hasCastle = true;
        } 
        if (canCastleBlackKing) {
            fen.append("k");
            hasCastle = true;
        }
        if (canCastleBlackQueen) {
            fen.append("q");
            hasCastle = true;
        } 

        if (!hasCastle) fen.append("-");


        // fen en passant
        fen.append(" ");
        if (enPassantTarget != -1) {
            fen.append(indexToAlgebraic(enPassantTarget));
        } else {
            fen.append("-");
        }
        
        // 50 move rule clock and full move clock (increment every black turn)
        fen.append(" " + turnsSinceCapture + " " + totalTurns);

        return fen.toString();
    }

    // index in array to algebraic notation i.e. e4 or d5 etc
    public String indexToAlgebraic(int index) {
        int file = index % 8;
        int rank = 8 - (index / 8);
        char fileChar = (char) ('a' + file);

        return "" + fileChar + rank;
    }

    private char getCharFromPiece(int piece) {
        switch (piece) {
            case 1: return 'P'; case 2: return 'N'; case 3: return 'B';
            case 4: return 'R'; case 5: return 'Q'; case 6: return 'K';
            case 9: return 'p'; case 10: return 'n'; case 11: return 'b';
            case 12: return 'r'; case 13: return 'q'; case 14: return 'k';
            default: return ' ';
        }
    }

    public int algebraicToIndex(String square) {
        int col = square.charAt(0) - 'a';
        int row = '8' - square.charAt(1);

        return row * 8 + col;
    }

    public void makeEngineMove(String moveString) {
        String startSquare = moveString.substring(0, 2);
        String endSquare = moveString.substring(2,4);

        int start = algebraicToIndex(startSquare);
        int end = algebraicToIndex(endSquare);

        int movingPiece = board[start];
        int capturedPiece = board[end];
        if (capturedPiece == 0 && movingPiece != 1 && movingPiece != 9) turnsSinceCapture++;
        else turnsSinceCapture = 0;

        if (capturedPiece == 4) {
            if (end == 63) canCastleWhiteKing = false;
            if (end == 56) canCastleWhiteQueen = false;
        } else if (capturedPiece == 12) {
            if (end == 7) canCastleBlackKing = false;
            if (end == 0) canCastleBlackQueen = false;
        }

        if ((movingPiece == 6 || movingPiece == 14) && Math.abs(end - start) == 2) {
            if (end > start) {
                castleKing(movingPiece == 6);
            } else {
                castleQueen(movingPiece == 6);
            }
        } else {
            if ((movingPiece == 1 || movingPiece == 9) && end == enPassantTarget && board[end] == 0) {
                int victimIndex = (movingPiece == 1) ? end + 8 : end - 8;
                board[victimIndex] = 0;
            }

            board[end] = board[start];
            board[start] = 0;
        }

        if (board[end] == 1 && end < 8) board[end] = 5;
        if (board[end] == 9 && end >= 56) board[end] = 13;

        if (movingPiece == 6) {
            canCastleWhiteKing = false;
            canCastleWhiteQueen = false;
        } else if (movingPiece == 14) {
            canCastleBlackKing = false;
            canCastleBlackQueen = false;
        } else if (movingPiece == 4) {
            if (start == 63) canCastleWhiteKing = false;
            if (start == 56) canCastleWhiteQueen = false;
        } else if (movingPiece == 12) {
            if (start == 7) canCastleBlackKing = false;
            if (start == 0) canCastleBlackQueen = false;
        }

        if (movingPiece == 1 && start - end == 16) {
            enPassantTarget = start - 8;
        } else if (movingPiece == 9 && end - start == 16) {
            enPassantTarget = start + 8;
        } else {
            enPassantTarget = -1;
        }

        currentTurn = (currentTurn == 1) ? -1 : 1;
        sourceTile = -1;
        lastMoveFrom = start;
        lastMoveTo = end;

        boolean irreversible = (capturedPiece != 0 || movingPiece == 1 || movingPiece == 9);
        recordPositionAndCheckDraw(irreversible);

        bridge.sendCommand("position fen " + getFen());
        bridge.sendCommand("checkmated");

        repaint();
    }
    public static void main(String[] args) {
        JFrame frame = new JFrame("Chess");
        CreateBoard boardPanel = new CreateBoard();

        frame.add(boardPanel);
        frame.setSize(720, 740);
        frame.setMinimumSize(new Dimension(420, 440));
        frame.getContentPane().setBackground(new Color(38, 36, 33));
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }
}
