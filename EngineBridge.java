import java.io.*;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

import javax.swing.JOptionPane;
import javax.swing.SwingUtilities;

public class EngineBridge {
    private Process engineProcess; // cpp connection
    private PrintWriter writer; // write/output to cpp
    private BufferedReader reader; // read/input from cpp
    private final BlockingQueue<String> bestMoves = new LinkedBlockingQueue<>();

    // executable path -> compiled exe or process file
    public EngineBridge(String executablePath) {
        try {
            engineProcess = new ProcessBuilder(executablePath).start();

             // setup output pipe    true => autoflush => immediately send the text
            writer = new PrintWriter(new OutputStreamWriter(engineProcess.getOutputStream()), true);

            // setup input pipe
            reader = new BufferedReader(new InputStreamReader(engineProcess.getInputStream()));
            System.out.println("working!");
        } catch (IOException e) {
            System.out.println("something wrong setup: " + e.getMessage());
        }
    }

    public void sendCommand(String command) {
        if (writer != null) {
            writer.println(command);
            writer.flush();
            
            if (!command.equals("go")) {
                System.out.println("java sent: " + command);
            }
        }
    }

    public void startListening(boolean first) {
        if (!first) return;
        // thread so ui doesn't stop updating
        Thread listenerThread = new Thread(() -> {
            try {
                String line;

                while ((line = reader.readLine()) != null) {
                    if (!line.contains("newl")) {
                        System.out.println("C++ says: " + line);
                    } else {
                        System.out.println(line);
                    }
                    if (line.startsWith("bestmove")) {
                        String[] parts = line.split(" ");
                        if (parts.length > 1) {
                            bestMoves.offer(parts[1]);
                        }
                    }

                    final String resultLine = line;
                    if (line.startsWith("result checkmate") || line.startsWith("result stalemate")) {
                        SwingUtilities.invokeLater(() -> {
                            JOptionPane.showMessageDialog(null, "Game Over: " + resultLine);
                        });
                    }
                }
            } catch (IOException e) {
                System.out.println("something wrong listener: " + e.getMessage());
            }
        });

        listenerThread.setDaemon(true);
        listenerThread.start();
    }

    public String waitForBestMove() {
        try {
            return bestMoves.take();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }
        return null;
    }

    public int[] moveToIndices(String move) {
        int startFile = move.charAt(0) - 'a';
        int startRank = 8 - Character.getNumericValue(move.charAt(1));
        int endFile = move.charAt(2) - 'a';
        int endRank = 8 - Character.getNumericValue(move.charAt(3));
        
        return new int[] { startRank * 8 + startFile, endRank * 8 + endFile };
    }
}
