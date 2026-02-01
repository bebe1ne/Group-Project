Live Gaming Leaderboard (Java, Min-Heap)
Files
LiveLeaderboard.java
Requirements
JDK (Java 8+ is fine)
No external libraries
Compile
In the directory containing LiveLeaderboard.java:

javac LiveLeaderboard.java
This creates LiveLeaderboard.class.

Run
java LiveLeaderboard
Commands (inside the program)
INIT <k>
Set the leaderboard size (Top K). Example:
INIT 3
SCORE <player> <score>
Submit a score entry. Example:
SCORE P1 100
SCORE P2 500
SCORE P3 300
SHOW_TOP
Show the current Top K entries, sorted by score (ascending).
EXIT
Quit the program.
Example session:

> INIT 3
> SCORE P1 100
> SCORE P2 500
> SCORE P3 300
> SCORE P4 50
> SCORE P5 200
> SHOW_TOP
> EXIT