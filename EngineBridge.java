import java.io.*;

public class EngineBridge {
    private Process engineProcess; // cpp connection
    private PrintWriter writer; // write/output to cpp
    private BufferedReader reader; // read/input from cpp

    public EngineBridge(String executablePath) {
        try {
            engineProcess = new ProcessBuilder(executablePath).start();

             // setup output pipe    true => autoflush => immediately send the text
            writer = new PrintWriter(new OutputStreamWriter(engineProcess.getOutputStream()), true);

            // setup input pipe
            reader = new BufferedReader(new InputStreamReader(engineProcess.getInputStream()));
            System.out.println("working!");
        } catch (IOException e) {
            System.out.println("somting wong in setup");
        }
    }

    public void sendCommand(String command) {
        if (writer != null) {
            writer.println(command);
            writer.flush();

            System.out.println("java sent: " + command);
        }
    }

    public void startListening() {
        // thread so ui doesn't stop updating
        Thread listenerThread = new Thread(() -> {
            try {
                String line;

                while ((line = reader.readLine()) != null) {
                    System.out.println("C++ says: " + line);
                    handleEngineResponse(line);
                }
            } catch (IOException e) {
                System.out.println("somting wong in listener");
            }
        });

        listenerThread.setDaemon(true);
        listenerThread.start();
    }

    private void handleEngineResponse(String response) {
        if (response.startsWith("bestmove")) {
            String move = response.split(" ")[1];
            System.out.println("engine says " + move + " is da best");
        }
    }

    // index in array to algebraic notation i.e. e4 or d5 etc
    public String indexToAlgebraic(int index) {
        int file = index % 8;
        int rank = 8 - (index / 8);
        char fileChar = (char) ('a' + file);

        return "" + fileChar + rank;
    }

    public int[] moveToIndices(String move) {
        int startFile = move.charAt(0) - 'a';
        int startRank = 8 - Character.getNumericValue(move.charAt(1));
        int endFile = move.charAt(2) - 'a';
        int endRank = 8 - Character.getNumericValue(move.charAt(3));
        
        return new int[] { startRank * 8 + startFile, endRank * 8 + endFile };
    }
}
