import javax.swing.*;
import java.awt.*;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.io.File;

public class CreateBoard extends JPanel {
    private final int TILESIZE = 80;
    private int selectedTile = -1;
    private int sourceTile = -1;

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

        addMouseListener(new MouseAdapter() {
            @Override
            public void mousePressed(MouseEvent e) {
                int col = e.getX() / TILESIZE;
                int row = e.getY() / TILESIZE;
                System.out.println("col: " + col + "row: " + row);

                if (col >= 0 && col < 8 && row >= 0 && row < 8) {
                    int clickedIndex = row * 8 + col;
                    
                    if (sourceTile == -1) {
                        if (board[clickedIndex] != 0) {
                            sourceTile = clickedIndex;
                        } 
                    } else {
                        if (clickedIndex == sourceTile) {
                            sourceTile = -1;
                        } else {
                            board[clickedIndex] = board[sourceTile];
                            board[sourceTile] = 0;

                            sourceTile = -1;
                        }
                    }
                }
                System.out.println("Selected: " + selectedTile);
                repaint();
            }
        });
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

            int pieceValue = board[i];
            if (pieceValue != 0 && pieceImages[pieceValue] != null) {
                g.drawImage(pieceImages[pieceValue], col * TILESIZE, row * TILESIZE, TILESIZE, TILESIZE, null);
            }
        }
    }

    public static void main(String[] args) {
        JFrame frame = new JFrame("chess display");
        frame.add(new CreateBoard());
        frame.setSize(640, 660); 
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setLocationRelativeTo(null);
        frame.setVisible(true);
    }
}