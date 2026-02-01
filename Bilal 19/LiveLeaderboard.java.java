import java.util.PriorityQueue;
import java.util.Comparator;
import java.util.Scanner;
import java.util.ArrayList;
import java.util.Collections;

public class LiveLeaderboard {

    One entry in the leaderboard
    static class Entry {
        String playerId;
        int score;

        Entry(String playerId, int score) {
            this.playerId = playerId;
            this.score = score;
        }

        @Override
        public String toString() {
            return "(" + playerId + ", " + score + ")";
        }
    }

    // Min-heap of size at most K (by score)
    private PriorityQueue<Entry> minHeap;
    private int K;

    public LiveLeaderboard() {
        this.K = 0;
        this.minHeap = null;
    }

    // Initialize leaderboard with size K
    public void init(int k) {
        if (k <= 0) {
            System.out.println("K must be positive.");
            return;
        }

        this.K = k;

        // Min-heap: compare entries by score ascending
        this.minHeap = new PriorityQueue<>(K, new Comparator<Entry>() {
            @Override
            public int compare(Entry e1, Entry e2) {
                return Integer.compare(e1.score, e2.score);
            }
        });

        System.out.println("Leaderboard initialized with size K = " + K);
    }

    // Process a new score submission
    public void submitScore(String playerId, int score) {
        if (minHeap == null || K == 0) {
            System.out.println("Leaderboard not initialized. Use INIT <k> first.");
            return;
        }

        // If heap not full, just insert
        if (minHeap.size() < K) {
            minHeap.offer(new Entry(playerId, score));
            System.out.println("Accepted: " + playerId + " " + score +
                               " (Heap size: " + minHeap.size() + ")");
            return;
        }

        // Heap full: compare with current smallest (root)
        Entry smallest = minHeap.peek();

        if (smallest != null && score > smallest.score) {
            // New score enters Top K, evict smallest
            minHeap.poll();
            minHeap.offer(new Entry(playerId, score));
            System.out.println("Accepted: " + playerId + " " + score +
                               " (evicted " + smallest.playerId + " " + smallest.score + ")");
        } else {
            // New score not high enough for Top K
            System.out.println("Ignored: " + playerId + " " + score +
                               " (<= current K-th best: " + smallest.score + ")");
        }
    }

    // Show current Top K (sorted by score ascending)
    public void showTop() {
        if (minHeap == null || K == 0) {
            System.out.println("Leaderboard not initialized. Use INIT <k> first.");
            return;
        }

        if (minHeap.isEmpty()) {
            System.out.println("Leaderboard is empty.");
            return;
        }

        // Copy heap to list and sort (heap iteration is not sorted)
        ArrayList<Entry> list = new ArrayList<>(minHeap);
        Collections.sort(list, new Comparator<Entry>() {
            @Override
            public int compare(Entry e1, Entry e2) {
                return Integer.compare(e1.score, e2.score);
            }
        });

        System.out.println("Current Top " + K + " (sorted by score):");
        for (Entry e : list) {
            System.out.println("  " + e.playerId + " : " + e.score);
        }
    }

    // Simple CLI
    public static void main(String[] args) {
        LiveLeaderboard leaderboard = new LiveLeaderboard();
        Scanner scanner = new Scanner(System.in);

        System.out.println("Live Gaming Leaderboard (Min-Heap Top K)");
        System.out.println("Commands:");
        System.out.println("  INIT <k>");
        System.out.println("  SCORE <player> <score>");
        System.out.println("  SHOW_TOP");
        System.out.println("  EXIT");

        while (true) {
            System.out.print("> ");
            if (!scanner.hasNextLine()) break;
            String line = scanner.nextLine().trim();
            if (line.isEmpty()) continue;

            String[] parts = line.split("\\s+");
            String command = parts[0].toUpperCase();

            if (command.equals("EXIT")) {
                System.out.println("Exiting.");
                break;
            } else if (command.equals("INIT")) {
                if (parts.length != 2) {
                    System.out.println("Usage: INIT <k>");
                    continue;
                }
                try {
                    int k = Integer.parseInt(parts[1]);
                    leaderboard.init(k);
                } catch (NumberFormatException e) {
                    System.out.println("K must be an integer.");
                }
            } else if (command.equals("SCORE")) {
                if (parts.length != 3) {
                    System.out.println("Usage: SCORE <player> <score>");
                    continue;
                }
                String player = parts[1];
                try {
                    int score = Integer.parseInt(parts[2]);
                    leaderboard.submitScore(player, score);
                } catch (NumberFormatException e) {
                    System.out.println("Score must be an integer.");
                }
            } else if (command.equals("SHOW_TOP")) {
                leaderboard.showTop();
            } else {
                System.out.println("Unknown command: " + command);
            }
        }

        scanner.close();
    }
}
