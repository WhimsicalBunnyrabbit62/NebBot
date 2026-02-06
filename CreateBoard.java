import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;

public class CreateBoard extends JPanel {
    private final int TILESIZE = 80;
    private int selectedTile = -1;
    private int sourceTile = -1;
    private int currentTurn = 1; // 1 white : -1 black
    private boolean canCastleWhite = true;
    private boolean canCastleBlack = true;
    private boolean castling = false;
    private int enPassantTarget = -1;
    private EngineBridge bridge = new EngineBridge("./chess_engine");

    private int[] board = {
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
        loadImages();


        //bridge.startListening();
        bridge.sendCommand("uci");
        bridge.sendCommand("isready");

        addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                int col = e.getX() / TILESIZE;
                int row = e.getY() / TILESIZE;
                //System.out.println("col: " + col + "row: " + row);
                
                if (currentTurn == 1) {
                    if (col >= 0 && col < 8 && row >= 0 && row < 8) {
                    int clickedIndex = row * 8 + col;
                    
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

                                    if ((board[sourceTile] == 1 || board[sourceTile] == 9) && clickedIndex == enPassantTarget) {
                                        int victimIndex = (currentTurn == 1) ? clickedIndex + 8 : clickedIndex - 8;
                                        board[victimIndex] = 0; 
                                    }

                                    if (castling) {
                                        if (clickedIndex == 58 || clickedIndex == 2) {
                                            castleQueen(currentTurn == 1);
                                        } else {
                                            castleKing(currentTurn == 1);
                                        }
                                    } else {
                                        board[clickedIndex] = board[sourceTile];
                                        board[sourceTile] = 0;
                                    }

                                    if (piece == 1 && sourceTile - clickedIndex == 16) {
                                        enPassantTarget = sourceTile - 8; 
                                    } else if (piece == 9 && clickedIndex - sourceTile == 16) {
                                        enPassantTarget = sourceTile + 8; 
                                    } else {
                                        enPassantTarget = -1; 
                                    }
                                    
                                    currentTurn = (currentTurn == 1) ? -1 : 1;
                                    castling = false;
                                } else {
                                    System.out.println("ya move wong");
                                }
                                
                                currentTurn = -1;
                                castling = false;
                                sourceTile = -1;
                                repaint();

                                runEngineTurn();
                            }
                        }
                    }
                } 
                
                //System.out.println("Selected: " + selectedTile);
                repaint();
            }
        });
    }

    private void runEngineTurn() {
        String fen = getFen();
        bridge.sendCommand("position fen " + fen);
        bridge.sendCommand("go");
        new Thread(() -> {
            String bestMove = bridge.waitForBestMove();

            if (bestMove == null || bestMove.contains("none")) {
                System.out.println("GAME OVER: Engine has no moves.");
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

    @Override
    protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        for (int i = 0; i < 64; i++) {
            int row = i / 8;
            int col = i % 8;

            g.setColor((row+col) % 2 == 0 ? new Color(235, 235, 208) : new Color(119, 148, 85));
            g.fillRect(col * TILESIZE, row * TILESIZE, TILESIZE, TILESIZE);

            if (i == selectedTile) {
                g.setColor(new Color(255, 255, 0, 128));
                g.fillRect(col * TILESIZE, row * TILESIZE, TILESIZE, TILESIZE);
            }

            if (i == sourceTile) {
                g.setColor(new Color(255, 255, 0, 150));
                g.fillRect(col * TILESIZE, row * TILESIZE, TILESIZE, TILESIZE);
            }

            int pieceValue = board[i];
            if (pieceValue != 0 && pieceImages[pieceValue] != null) {
                g.drawImage(pieceImages[pieceValue], col * TILESIZE, row * TILESIZE, TILESIZE, TILESIZE, null);
            }
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
                    
                    if (end == enPassantTarget) return true;
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
                if (Math.abs((start % 8) - (end % 8)) == 1 && board[end] != 0) return true;
            }

            if (end - start == 7 || end - start == 9) {
                if (colDiff == 1) {
                    if (board[end] != 0) return true;
                    
                    if (end == enPassantTarget) return true;
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

            if (piece == 6 && canCastleWhite && start == 60) { // White
                if (end == 62 && board[61] == 0 && board[62] == 0) { 
                    castling = true;
                    canCastleWhite = false; 
                    return true;
                }
                if (end == 58 && board[59] == 0 && board[58] == 0 && board[57] == 0) { 
                    castling = true;
                    canCastleWhite = false;
                    return true;
                }
            }

            if (piece == 14 && canCastleBlack && start == 4) { // Black
                if (end == 6 && board[5] == 0 && board[6] == 0) { 
                    castling = true;
                    canCastleBlack = false;
                    return true;
                }
                if (end == 2 && board[3] == 0 && board[2] == 0 && board[1] == 0) {
                    castling = true;
                    canCastleBlack = false;
                    return true;
                }
            }

            if (colDiff <= 1 && rowDiff <= 1) {
                if (piece == 6) {
                    canCastleWhite = false;
                } else {
                    canCastleBlack = false;
                }
            }

            return colDiff <= 1 && rowDiff <= 1;
        }

        return true;
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
        if (canCastleWhite) {
            fen.append("KQ");
            hasCastle = true;
        } 
        if (canCastleBlack) {
            fen.append("kq");
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
        fen.append(" 0 1");

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

        board[end] = board[start];
        board[start] = 0;

        if (board[end] == 1 && end < 8) board[end] = 5;
        if (board[end] == 9 && end >= 56) board[end] = 13;

        currentTurn = 1;
        sourceTile = -1;

        repaint();
    }
    public static void main(String[] args) {
        JFrame frame = new JFrame("chess display");
        CreateBoard boardPanel = new CreateBoard();
        frame.add(boardPanel);
        frame.setSize(640, 660); 
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }
}