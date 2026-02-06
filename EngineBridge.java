import java.io.*;

public class EngineBridge {
    private Process engineProcess; // cpp connection
    private PrintWriter writer; // write/output to cpp
    private BufferedReader reader; // read/input from cpp

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
                System.out.println("something wrong listener: " + e.getMessage());
            }
        });

        listenerThread.setDaemon(true);
        listenerThread.start();
    }

    public String waitForBestMove() {
        try {
            String line;
            while ((line = reader.readLine()) != null) {
                System.out.println("C++ says: " + line); 
                
                if (line.startsWith("bestmove")) {
                    return line.split(" ")[1];
                }
            }
        } catch (IOException e) {
            System.out.println("Error reading from engine: " + e.getMessage());
        }
        return null;
    }

    private void handleEngineResponse(String response) {
        if (response.startsWith("bestmove")) {
            String move = response.split(" ")[1];
            System.out.println("engine says " + move + " is da best");
        }
    }

    public int[] moveToIndices(String move) {
        int startFile = move.charAt(0) - 'a';
        int startRank = 8 - Character.getNumericValue(move.charAt(1));
        int endFile = move.charAt(2) - 'a';
        int endRank = 8 - Character.getNumericValue(move.charAt(3));
        
        return new int[] { startRank * 8 + startFile, endRank * 8 + endFile };
    }
}
