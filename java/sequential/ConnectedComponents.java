package sequential;

import java.io.*;
import java.util.*;

public class ConnectedComponents {

    static int[][] readPGM(String filename) throws IOException {
        BufferedReader br = new BufferedReader(new FileReader(filename));
        String magic = br.readLine().trim();
        if (!magic.equals("P2")) throw new IOException("Not a P2 PGM file: " + magic);
        String line;
        while ((line = br.readLine()) != null && line.startsWith("#")) {}
        Scanner sc = new Scanner(line);
        int width = sc.nextInt();
        int height = sc.nextInt();
        br.readLine(); // maxval
        int[][] pixels = new int[height][width];
        StreamTokenizer st = new StreamTokenizer(br);
        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++) {
                st.nextToken();
                pixels[i][j] = (int) st.nval;
            }
        br.close();
        return pixels;
    }

    static void writePGM(String filename, int[][] pixels, int maxval) throws IOException {
        int height = pixels.length;
        int width = pixels[0].length;
        PrintWriter pw = new PrintWriter(new FileWriter(filename));
        pw.println("P2");
        pw.println(width + " " + height);
        pw.println(maxval);
        for (int i = 0; i < height; i++) {
            StringBuilder sb = new StringBuilder();
            for (int j = 0; j < width; j++) {
                if (j > 0) sb.append(' ');
                sb.append(pixels[i][j]);
            }
            pw.println(sb.toString());
        }
        pw.close();
    }

    static int[] dr = {-1, 1, 0, 0};
    static int[] dc = {0, 0, -1, 1};

    static int connectedComponents(int[][] input, int[][] output) {
        int height = input.length;
        int width = input[0].length;
        int[][] labels = new int[height][width];
        for (int[] row : labels) Arrays.fill(row, -1);

        int numComp = 0;
        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                if (input[i][j] > 127 && labels[i][j] == -1) {
                    Queue<int[]> queue = new LinkedList<>();
                    queue.add(new int[]{i, j});
                    labels[i][j] = numComp;
                    while (!queue.isEmpty()) {
                        int[] cur = queue.poll();
                        int r = cur[0], c = cur[1];
                        for (int d = 0; d < 4; d++) {
                            int nr = r + dr[d];
                            int nc = c + dc[d];
                            if (nr >= 0 && nr < height && nc >= 0 && nc < width
                                    && input[nr][nc] > 127 && labels[nr][nc] == -1) {
                                labels[nr][nc] = numComp;
                                queue.add(new int[]{nr, nc});
                            }
                        }
                    }
                    numComp++;
                }
            }
        }

        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++) {
                if (labels[i][j] == -1) output[i][j] = 0;
                else if (numComp <= 1) output[i][j] = 255;
                else output[i][j] = (labels[i][j] * 255) / (numComp - 1);
            }

        return numComp;
    }

    public static void main(String[] args) throws IOException {
        if (args.length < 2) {
            System.err.println("Usage: java sequential.ConnectedComponents <input.pgm> <output.pgm>");
            System.exit(1);
        }

        int[][] pixels = readPGM(args[0]);
        int height = pixels.length, width = pixels[0].length;
        System.out.println("Image loaded: " + width + "x" + height);

        int[][] output = new int[height][width];
        long start = System.currentTimeMillis();
        int numComp = connectedComponents(pixels, output);
        long end = System.currentTimeMillis();

        System.out.println("Connected components found: " + numComp);
        System.out.println("Connected components (sequential) time: " + (end - start) + " ms");
        writePGM(args[1], output, 255);
        System.out.println("Output written to " + args[1]);
    }
}
