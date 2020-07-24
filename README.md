# Othello AI

This is an Othello AI program that implements a minimax algorithm, with alpha beta pruning, and that makes use of MPI parallelization, and other techniques such as iterative deepening, and an advanced evaluation function to choose the best move possible. 
s
# How it works

The whole game process is controlled by the master process (process 0), when it is our AI's turn to make a move, the master process will synchronize the current board in play with the other processes, and then send a message to the other processes, asking for each to return a move. Each process then gets an initial location based on their rank, and runs the minimax algorithm on these initial locations with a depth of 4. If more time is available after minimax returns it's results (Just under the time limit specified by the game engine, allows for extra time for values to be returned.) it increases the depth by 1 and runs minimax again on this new depth. 

The child processes will then all send their initial moves along with their scores back to process 0, and process 0 will then in turn choose the best score out of the returned scores and play that best move.

# Evaluation function

Loops over the board and adds points to the specific player depending where
they played.
 Points Distribution:
 - 1 Point for a normal inner board space.
 - 5 Points for an edge board space.
 - 10 Points for a corner board space.
 
 The opponent's score is then subtracted from my score, as to obtain the move that results in the greatest lead between the two players.

  # Acknowledgements
  The following people or resources have helped me whilst coding this project:
  - Andrew Cullis
  - Jaco Swart
  - Thomas Milligan
  - Liza O'Kennedy 

The above mentioned were involved in discussions on implementation/logic behind algorithm and general program structure.
The following YouTube video was used for understanding of the minimax algorithm, which included pseudocode for the aforementioned algorithm:
https://www.youtube.com/watch?v=l-hh51ncgDI&list=WL&index=54&t=0s
with link to pseudocode:
https://pastebin.com/rZg1Mz9G

I was also provided with skeleton code.


# Author

Matthew Aiden Almano - 21338418
