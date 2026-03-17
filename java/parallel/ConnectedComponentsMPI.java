package parallel;

import mpi.*;
import java.io.*;
import java.util.*;

public class ConnectedComponentsMPI {

    static int[] readPGMFlat(String filename, int[] dims) throws IOException {
        BufferedReader br = new BufferedReader(new FileReader(filename));
        String magic = br.readLine().trim();
        if (!magic.equals("P2")) throw new IOException("Not a P2 PGM file");
        String line;
        while ((line = br.readLine()) != null && line.startsWith("#")) {}
        int width, height;
        try (Scanner sc = new Scanner(line)) {
            width = sc.nextInt();
            height = sc.nextInt();
        }
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

    // Simple Union-Find using int arrays
    static int[] ufParent;
    static int ufFind(int x) {
        if (ufParent[x] != x) ufParent[x] = ufFind(ufParent[x]);
        return ufParent[x];
    }
    static void ufUnite(int a, int b) {
        ufParent[ufFind(a)] = ufFind(b);
    }

    static final int[] DR = {-1, 1, 0, 0};
    static final int[] DC = {0, 0, -1, 1};

    static int localBFS(int[] pixels, int width, int startRow, int numRows, int[] labels, int labelOffset) {
        Arrays.fill(labels, 0, numRows * width, -1);
        int numComp = 0;

        for (int i = 0; i < numRows; i++) {
            for (int j = 0; j < width; j++) {
                if (pixels[(startRow + i) * width + j] > 127 && labels[i * width + j] == -1) {
                    int lbl = labelOffset + numComp;
                    Queue<int[]> q = new LinkedList<>();
                    q.add(new int[]{i, j});
                    labels[i * width + j] = lbl;
                    while (!q.isEmpty()) {
                        int[] cur = q.poll();
                        int r = cur[0], c = cur[1];
                        for (int d = 0; d < 4; d++) {
                            int nr = r + DR[d], nc = c + DC[d];
                            if (nr >= 0 && nr < numRows && nc >= 0 && nc < width
                                    && pixels[(startRow + nr) * width + nc] > 127
                                    && labels[nr * width + nc] == -1) {
                                labels[nr * width + nc] = lbl;
                                q.add(new int[]{nr, nc});
                            }
                        }
                    }
                    numComp++;
                }
            }
        }
        return numComp;
    }

    public static void main(String[] args) throws Exception {
        MPI.Init(args);
        int rank = MPI.COMM_WORLD.Rank();
        int size = MPI.COMM_WORLD.Size();

        String inputFile = null, outputFile = null;
        for (int i = 0; i < args.length; i++) {
            if (!args[i].startsWith("-")) {
                if (inputFile == null) inputFile = args[i];
                else if (outputFile == null) { outputFile = args[i]; break; }
            }
        }

        if (inputFile == null || outputFile == null) {
            if (rank == 0) System.err.println("Usage: ConnectedComponentsMPI <input.pgm> <output.pgm>");
            MPI.Finalize();
            System.exit(1);
        }

        int[] dims = new int[3];
        int[] allPixels;

        if (rank == 0) {
            allPixels = readPGMFlat(inputFile, dims);
            System.out.println("Image loaded: " + dims[0] + "x" + dims[1]);
        } else {
            allPixels = new int[0];
        }

        MPI.COMM_WORLD.Bcast(dims, 0, 3, MPI.INT, 0);
        int width = dims[0], height = dims[1];

        if (rank != 0) allPixels = new int[width * height];
        MPI.COMM_WORLD.Bcast(allPixels, 0, width * height, MPI.INT, 0);

        int baseRows = height / size;
        int extra = height % size;
        int myRows = baseRows + (rank < extra ? 1 : 0);
        int startRow = rank * baseRows + Math.min(rank, extra);

        int[] rowCounts = new int[size];
        MPI.COMM_WORLD.Allgather(new int[]{myRows}, 0, 1, MPI.INT, rowCounts, 0, 1, MPI.INT);
        int labelOffset = 0;
        for (int i = 0; i < rank; i++) labelOffset += rowCounts[i] * width;

        int maxLabels = width * height;
        ufParent = new int[maxLabels];
        for (int i = 0; i < maxLabels; i++) ufParent[i] = i;

        long startTime = System.currentTimeMillis();

        int[] localLabels = new int[myRows * width];
        localBFS(allPixels, width, startRow, myRows, localLabels, labelOffset);

        // Exchange boundary rows with neighbors
        if (rank < size - 1) {
            int nextRank = rank + 1;
            int[] myBottomPixels = Arrays.copyOfRange(allPixels, (startRow + myRows - 1) * width, (startRow + myRows) * width);
            int[] myBottomLabels = Arrays.copyOfRange(localLabels, (myRows - 1) * width, myRows * width);
            int[] theirTopPixels = new int[width];
            int[] theirTopLabels = new int[width];

            MPI.COMM_WORLD.Sendrecv(myBottomPixels, 0, width, MPI.INT, nextRank, 0,
                    theirTopPixels, 0, width, MPI.INT, nextRank, 1, MPI.COMM_WORLD);
            MPI.COMM_WORLD.Sendrecv(myBottomLabels, 0, width, MPI.INT, nextRank, 2,
                    theirTopLabels, 0, width, MPI.INT, nextRank, 3, MPI.COMM_WORLD);

            for (int j = 0; j < width; j++) {
                if (myBottomPixels[j] > 127 && theirTopPixels[j] > 127
                        && myBottomLabels[j] != -1 && theirTopLabels[j] != -1) {
                    ufUnite(myBottomLabels[j], theirTopLabels[j]);
                }
            }
        }

        if (rank > 0) {
            int prevRank = rank - 1;
            int[] myTopPixels = Arrays.copyOfRange(allPixels, startRow * width, (startRow + 1) * width);
            int[] myTopLabels = Arrays.copyOfRange(localLabels, 0, width);
            int[] theirBottomPixels = new int[width];
            int[] theirBottomLabels = new int[width];

            MPI.COMM_WORLD.Sendrecv(myTopPixels, 0, width, MPI.INT, prevRank, 1,
                    theirBottomPixels, 0, width, MPI.INT, prevRank, 0, MPI.COMM_WORLD);
            MPI.COMM_WORLD.Sendrecv(myTopLabels, 0, width, MPI.INT, prevRank, 3,
                    theirBottomLabels, 0, width, MPI.INT, prevRank, 2, MPI.COMM_WORLD);

            for (int j = 0; j < width; j++) {
                if (myTopPixels[j] > 127 && theirBottomPixels[j] > 127
                        && myTopLabels[j] != -1 && theirBottomLabels[j] != -1) {
                    ufUnite(myTopLabels[j], theirBottomLabels[j]);
                }
            }
        }

        for (int idx = 0; idx < localLabels.length; idx++) {
            if (localLabels[idx] != -1) localLabels[idx] = ufFind(localLabels[idx]);
        }

        long endTime = System.currentTimeMillis();

        int[] recvCounts = new int[size];
        int[] displs = new int[size];
        for (int i = 0; i < size; i++) {
            int r = baseRows + (i < extra ? 1 : 0);
            recvCounts[i] = r * width;
            displs[i] = (i * baseRows + Math.min(i, extra)) * width;
        }

        int[] allLabels = null;
        if (rank == 0) allLabels = new int[width * height];

        MPI.COMM_WORLD.Gatherv(localLabels, 0, myRows * width, MPI.INT,
                allLabels, 0, recvCounts, displs, MPI.INT, 0);

        if (rank == 0) {
            Map<Integer, Integer> labelMap = new HashMap<>();
            int compCount = 0;
            int[] outPixels = new int[width * height];
            for (int i = 0; i < width * height; i++) {
                if (allLabels[i] != -1) {
                    int root = allLabels[i];
                    if (!labelMap.containsKey(root)) labelMap.put(root, ++compCount);
                    outPixels[i] = labelMap.get(root);
                }
            }
            for (int i = 0; i < width * height; i++) {
                if (allLabels[i] == -1) outPixels[i] = 0;
                else if (compCount <= 1) outPixels[i] = 255;
                else outPixels[i] = (outPixels[i] * 255) / compCount;
            }

            long elapsed = endTime - startTime;
            System.out.println("Connected components found: " + compCount);
            System.out.println("Connected components (parallel, " + size + " procs) time: " + elapsed + " ms");
            writePGM(outputFile, outPixels, width, height, 255);
            System.out.println("Output written to " + outputFile);
        }

        MPI.Finalize();
    }
}
