package parallel;

import mpi.*;
import java.io.*;
import java.util.*;

public class MedianFilterMPI {

    static int[] readPGMFlat(String filename, int[] dims) throws IOException {
        BufferedReader br = new BufferedReader(new FileReader(filename));
        String magic = br.readLine().trim();
        if (!magic.equals("P2")) throw new IOException("Not a P2 PGM file");
        String line;
        while ((line = br.readLine()) != null && line.startsWith("#")) {}
        Scanner sc = new Scanner(line);
        int width = sc.nextInt();
        int height = sc.nextInt();
        int maxval = Integer.parseInt(br.readLine().trim());
        dims[0] = width; dims[1] = height; dims[2] = maxval;
        int[] pixels = new int[width * height];
        StreamTokenizer st = new StreamTokenizer(br);
        for (int i = 0; i < width * height; i++) {
            st.nextToken();
            pixels[i] = (int) st.nval;
        }
        br.close();
        return pixels;
    }

    static void writePGM(String filename, int[] pixels, int width, int height, int maxval) throws IOException {
        PrintWriter pw = new PrintWriter(new FileWriter(filename));
        pw.println("P2");
        pw.println(width + " " + height);
        pw.println(maxval);
        for (int i = 0; i < height; i++) {
            StringBuilder sb = new StringBuilder();
            for (int j = 0; j < width; j++) {
                if (j > 0) sb.append(' ');
                sb.append(pixels[i * width + j]);
            }
            pw.println(sb.toString());
        }
        pw.close();
    }

    static int[] filterRows(int[] data, int width, int localRows, int half) {
        int totalRows = localRows + 2 * half;
        int[] result = new int[localRows * width];
        int kSize = (2 * half + 1) * (2 * half + 1);
        int[] neighbors = new int[kSize];

        for (int i = 0; i < localRows; i++) {
            int actualI = i + half;
            for (int j = 0; j < width; j++) {
                int k = 0;
                for (int ki = -half; ki <= half; ki++) {
                    for (int kj = -half; kj <= half; kj++) {
                        int ni = actualI + ki;
                        int nj = Math.max(0, Math.min(width - 1, j + kj));
                        if (ni < 0) ni = 0;
                        if (ni >= totalRows) ni = totalRows - 1;
                        neighbors[k++] = data[ni * width + nj];
                    }
                }
                Arrays.sort(neighbors, 0, k);
                result[i * width + j] = neighbors[k / 2];
            }
        }
        return result;
    }

    public static void main(String[] args) throws Exception {
        MPI.Init(args);
        int rank = MPI.COMM_WORLD.Rank();
        int size = MPI.COMM_WORLD.Size();

        // Remove MPI args; actual program args are passed differently in MPJ
        // args[0] = input, args[1] = output
        String inputFile = null, outputFile = null;
        for (int i = 0; i < args.length; i++) {
            if (!args[i].startsWith("-")) {
                if (inputFile == null) inputFile = args[i];
                else if (outputFile == null) { outputFile = args[i]; break; }
            }
        }

        if (inputFile == null || outputFile == null) {
            if (rank == 0) System.err.println("Usage: MedianFilterMPI <input.pgm> <output.pgm>");
            MPI.Finalize();
            System.exit(1);
        }

        int[] dims = new int[3];
        int[] allPixels = null;

        if (rank == 0) {
            allPixels = readPGMFlat(inputFile, dims);
            System.out.println("Image loaded: " + dims[0] + "x" + dims[1]);
        } else {
            allPixels = new int[0];
        }

        MPI.COMM_WORLD.Bcast(dims, 0, 3, MPI.INT, 0);
        int width = dims[0], height = dims[1], maxval = dims[2];

        if (rank != 0) allPixels = new int[width * height];
        MPI.COMM_WORLD.Bcast(allPixels, 0, width * height, MPI.INT, 0);

        int baseRows = height / size;
        int extra = height % size;
        int myRows = baseRows + (rank < extra ? 1 : 0);
        int startRow = rank * baseRows + Math.min(rank, extra);

        int half = 1;
        int[] localData = new int[(myRows + 2 * half) * width];
        for (int i = -half; i < myRows + half; i++) {
            int srcRow = startRow + i;
            srcRow = Math.max(0, Math.min(height - 1, srcRow));
            int dstIdx = (i + half) * width;
            System.arraycopy(allPixels, srcRow * width, localData, dstIdx, width);
        }

        long startTime = System.currentTimeMillis();
        int[] localResult = filterRows(localData, width, myRows, half);
        long endTime = System.currentTimeMillis();

        int[] recvCounts = new int[size];
        int[] displs = new int[size];
        for (int i = 0; i < size; i++) {
            int r = baseRows + (i < extra ? 1 : 0);
            recvCounts[i] = r * width;
            displs[i] = (i * baseRows + Math.min(i, extra)) * width;
        }

        int[] result = null;
        if (rank == 0) result = new int[width * height];

        MPI.COMM_WORLD.Gatherv(localResult, 0, myRows * width, MPI.INT,
                result, 0, recvCounts, displs, MPI.INT, 0);

        if (rank == 0) {
            long elapsed = endTime - startTime;
            System.out.println("Median filter (parallel, " + size + " procs) time: " + elapsed + " ms");
            writePGM(outputFile, result, width, height, maxval);
            System.out.println("Output written to " + outputFile);
        }

        MPI.Finalize();
    }
}
