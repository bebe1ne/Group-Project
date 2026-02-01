import java.util.*;

/**
 * Build Dependency Resolver using Topological Sort (Kahn's Algorithm).
 *
 * Commands:
 *   DEPENDS <A> <B>  -> A depends on B (edge: B -> A)
 *   BUILD
 *   EXIT
 */
public class BuildDependencyResolver {

    // Adjacency list: node -> list of nodes that depend on it (outgoing edges)
    private Map<String, List<String>> graph;

    // In-degree: node -> number of dependencies (incoming edges)
    private Map<String, Integer> inDegree;

    public BuildDependencyResolver() {
        this.graph = new HashMap<>();
        this.inDegree = new HashMap<>();
    }

    // Add dependency A depends on B (edge: B -> A)
    public void addDependency(String A, String B) {
        ensureNode(A);
        ensureNode(B);

        graph.get(B).add(A);                        // edge B -> A
        inDegree.put(A, inDegree.get(A) + 1);       // A has one more dependency

        System.out.println("Added dependency: " + A + " depends on " + B +
                           " (edge " + B + " -> " + A + ")");
    }

    // Ensure node exists in graph and inDegree
    private void ensureNode(String node) {
        graph.putIfAbsent(node, new ArrayList<>());
        inDegree.putIfAbsent(node, 0);
    }

    // Perform topological sort, print build order or cycle error
    public void build() {
        if (graph.isEmpty()) {
            System.out.println("No files to build.");
            return;
        }

        Queue<String> queue = new LinkedList<>();

        // Add all nodes with in-degree 0
        for (String node : inDegree.keySet()) {
            if (inDegree.get(node) == 0) {
                queue.offer(node);
            }
        }

        if (queue.isEmpty()) {
            System.out.println("ERROR: Circular dependency detected (no starting point).");
            return;
        }

        List<String> buildOrder = new ArrayList<>();

        // Kahn's algorithm
        while (!queue.isEmpty()) {
            String current = queue.poll();
            buildOrder.add(current);

            for (String neighbor : graph.get(current)) {
                int updated = inDegree.get(neighbor) - 1;
                inDegree.put(neighbor, updated);
                if (updated == 0) {
                    queue.offer(neighbor);
                }
            }
        }

        // If not all nodes were processed, we have a cycle
        if (buildOrder.size() < inDegree.size()) {
            System.out.println("ERROR: Circular dependency detected.");
        } else {
            System.out.println("Build order:");
            int step = 1;
            for (String file : buildOrder) {
                System.out.println(step + ". " + file);
                step++;
            }
        }

        // Restore inDegree for future BUILD commands
        recomputeInDegree();
    }

    // Recompute inDegree from adjacency list
    private void recomputeInDegree() {
        // Reset all to 0
        for (String node : inDegree.keySet()) {
            inDegree.put(node, 0);
        }

        // Count incoming edges
        for (String u : graph.keySet()) {
            for (String v : graph.get(u)) {
                inDegree.put(v, inDegree.get(v) + 1);
            }
        }
    }

    // Simple CLI
    public static void main(String[] args) {
        BuildDependencyResolver resolver = new BuildDependencyResolver();
        Scanner scanner = new Scanner(System.in);

        System.out.println("Build Dependency Resolver (Topological Sort)");
        System.out.println("Commands:");
        System.out.println("  DEPENDS <A> <B>   (A depends on B, edge: B -> A)");
        System.out.println("  BUILD");
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
            } else if (command.equals("DEPENDS")) {
                if (parts.length != 3) {
                    System.out.println("Usage: DEPENDS <A> <B>");
                    continue;
                }
                String A = parts[1];
                String B = parts[2];
                resolver.addDependency(A, B);
            } else if (command.equals("BUILD")) {
                resolver.build();
            } else {
                System.out.println("Unknown command: " + command);
            }
        }

        scanner.close();
    }
}
