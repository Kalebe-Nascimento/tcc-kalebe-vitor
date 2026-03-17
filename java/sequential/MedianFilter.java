package sequential;

import java.io.*;
import java.util.*;

public class MedianFilter {

    static int[][] readPGM(String filename) throws IOException {
        BufferedReader br = new BufferedReader(new FileReader(filename));
        String magic = br.readLine().trim();
        if (!magic.equals("P2")) throw new IOException("Not a P2 PGM file: " + magic);

        String line;
        // Skip comments
        while ((line = br.readLine()) != null && line.startsWith("#")) {}
        // line now has width height
        int width, height;
        try (Scanner sc = new Scanner(line)) {
            width = sc.nextInt();
            height = sc.nextInt();
        }
        int maxval = Integer.parseInt(br.readLine().trim());

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

    static int[][] applyMedianFilter(int[][] input, int kernelSize) {
        int height = input.length;
        int width = input[0].length;
        int half = kernelSize / 2;
        int[][] output = new int[height][width];
        int[] neighbors = new int[kernelSize * kernelSize];

        for (int i = 0; i < height; i++) {
            for (int j = 0; j < width; j++) {
                int k = 0;
                for (int ki = -half; ki <= half; ki++) {
                    for (int kj = -half; kj <= half; kj++) {
                        int ni = Math.max(0, Math.min(height - 1, i + ki));
                        int nj = Math.max(0, Math.min(width - 1, j + kj));
                        neighbors[k++] = input[ni][nj];
                    }
                }
                Arrays.sort(neighbors, 0, k);
                output[i][j] = neighbors[k / 2];
            }
        }
        return output;
    }

    public static void main(String[] args) throws IOException {
        if (args.length < 2) {
            System.err.println("Usage: java sequential.MedianFilter <input.pgm> <output.pgm>");
            System.exit(1);
        }

        int[][] pixels = readPGM(args[0]);
        System.out.println("Image loaded: " + pixels[0].length + "x" + pixels.length);

        long start = System.currentTimeMillis();
        int[][] result = applyMedianFilter(pixels, 3);
        long end = System.currentTimeMillis();

        System.out.println("Median filter (sequential) time: " + (end - start) + " ms");
        writePGM(args[1], result, 255);
        System.out.println("Output written to " + args[1]);
    }
}
