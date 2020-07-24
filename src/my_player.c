/*H**********************************************************************
 *
 *	This is a skeleton to guide development of Othello engines to be used
 *	with the Cloud Tournament Engine (CTE). CTE runs tournaments between
 *	game engines and displays the results on a web page which is available on
 *	campus (!!!INSERT URL!!!). Any output produced by your implementations will
 *	be available for download on the web page.
 *
 *	The socket communication required for DTE is handled in the main method,
 *	which is provided with the skeleton. All socket communication is performed
 *	at rank 0.
 *
 *	Board co-ordinates for moves start at the top left corner of the board i.e.
 *	if your engine wishes to place a piece at the top left corner, the "gen_move"
 *	function must return "00".
 *
 *	The match is played by making alternating calls to each engine's "gen_move"
 *	and "play_move" functions. The progression of a match is as follows:
 *		1. Call gen_move for black player
 *		2. Call play_move for white player, providing the black player's move
 *		3. Call gen move for white player
 *		4. Call play_move for black player, providing the white player's move
 *		.
 *		.
 *		.
 *		N. A player makes the final move and "game_over" is called for both players
 *	
 *	IMPORTANT NOTE:
 *		Any output that you would like to see (for debugging purposes) needs
 *		to be written to file. This can be done using file FP, and fprintf(),
 *		don't forget to flush the stream. 
 *		I would suggest writing a method to make this
 *		easier, I'll leave that to you.
 *		The file name is passed as argv[4], feel free to change to whatever suits you.
 *H***********************************************************************/

#include<stdio.h>
#include<limits.h>
#include<stdlib.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<mpi.h>
#include<time.h>

const int EMPTY = 0;
const int BLACK = 1;
const int WHITE = 2;
const int OUTER = 3;
const int ALLDIRECTIONS[8]={-11, -10, -9, -1, 1, 9, 10, 11};
const int BOARDSIZE=100;
const int MOVE_TAG = 0;
const double TIME_OFFSET = 0.3;
const int MAX_DEPTH = 30;
int REQUEST_MOVE = 1;

int retrieve_move(int rank);
int run_minimax(int loc, int depth, int alpha, int beta, int maxPlayer);
void play_move(char *move);
void game_over();
void request_move();
void run_worker(int rank);
void initialise();

int* initialboard(void);
int *legalmoves ();
int legalp (int move, int player);
int validp (int move);
int wouldflip (int move, int dir, int player);
int opponent (int player);
int findbracketingpiece(int square, int dir, int player);
void makemove (int move, int player);
void makeflips (int move, int dir, int player);
int get_loc(char* movestring);
char* get_move_string(int loc);
void printboard();
char nameof(int piece);
int count (int player, int * board);
void sync_board();
int max(int a, int b);
int min(int a, int b);

int my_colour;
double time_limit;
double start_time;
int running;
int rank;
int size;
int *board;
int firstrun = 1;
FILE *fp;
int main(int argc , char *argv[]) {
    int socket_desc, port, msg_len;
    char *ip, *cmd, *opponent_move, *my_move;
    char msg_buf[15], len_buf[2];
    struct sockaddr_in server;
    ip = argv[1];
    port = atoi(argv[2]);
    time_limit = atoi(argv[3]);
    my_colour = EMPTY;
    running = 1;
    /* starts MPI */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);	/* get current process id */
    MPI_Comm_size(MPI_COMM_WORLD, &size);	/* get number of processes */
    // Rank 0 is responsible for handling communication with the server
    if (rank == 0){
       

        fp = fopen(argv[4], "w");
        fprintf(fp, "This is an example of output written to file.\n");
        fflush(fp);
        initialise();
    
        socket_desc = socket(AF_INET , SOCK_STREAM , 0);
        if (socket_desc == -1) {
            fprintf(fp, "Could not create socket\n");
            fflush(fp);
            return -1;
        }
        server.sin_addr.s_addr = inet_addr(ip);
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        //Connect to remote server
        if (connect(socket_desc , (struct sockaddr *)&server , sizeof(server)) < 0){

            fprintf(fp, "Connect error\n");
            fflush(fp);
            return -1;
        }
        fprintf(fp, "Connected\n");
        fflush(fp);
        if (socket_desc == -1){
            return 1;
        }
        while (running == 1){
            if (firstrun ==1) {
                
                char tempColour[1];
                if(recv(socket_desc, tempColour , 1, 0) < 0){
                    fprintf(fp,"Receive failed\n");
                    fflush(fp);
                    running = 0;
                    break;
                }
                my_colour = atoi(tempColour);
                fprintf(fp,"Player colour is: %d\n", my_colour);
                fflush(fp);
                firstrun = 2;
                sync_board();
            }


            if(recv(socket_desc, len_buf , 2, 0) < 0){
                fprintf(fp,"Receive failed\n");
                fflush(fp);
                running = 0;
                break;
            }

            msg_len = atoi(len_buf);


            if(recv(socket_desc, msg_buf, msg_len, 0) < 0){
                fprintf(fp,"Receive failed\n");
                fflush(fp);
                running = 0;
                break;
            }


            msg_buf[msg_len] = '\0';
            cmd = strtok(msg_buf, " ");

            if (strcmp(cmd, "game_over") == 0){
                running = 0;
                fprintf(fp, "Game over\n");
                fflush(fp);
                sync_board();
                break;

            } else if (strcmp(cmd, "gen_move") == 0){
                int recv_loc,i, my_loc, loc_score;
                int my_locs[size];
                int loc_scores[size];
                my_loc = -1;
                loc_score = 0;
                sync_board();
                request_move();
                for(i = 1; i < size; i++){
                    MPI_Recv(&recv_loc, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Recv(&loc_score, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    my_locs[i] = recv_loc;
                    loc_scores[i] = loc_score;
                }
                
                for(i = 1; i < size; i++){
                    if(loc_scores[i] >= loc_score && my_locs[i] != -1){
                        loc_score = loc_scores[i];
                        my_loc = my_locs[i];
                    }
                }

                if(my_loc == -1){
                    my_move = "pass\n";
                }else{
                    my_move = get_move_string(my_loc);
                }
                makemove(my_loc, my_colour);
                if (send(socket_desc, my_move, strlen(my_move) , 0) < 0){
                    running = 0;
                    fprintf(fp,"Move send failed\n");
                    fflush(fp);
                    break;
                }
                printboard();
            } else if (strcmp(cmd, "play_move") == 0){
                opponent_move = strtok(NULL, " ");
                play_move(opponent_move);
                printboard();

            }
            
            memset(len_buf, 0, 2);
            memset(msg_buf, 0, 15);
        }
        game_over();
    } else {
        run_worker(rank);
        MPI_Finalize();
    }
    return 0;
}

/*
	Called at the start of execution on all ranks
 */
void initialise(){
    int i;
    running = 1;
    board = (int *)malloc(BOARDSIZE * sizeof(int));
    for (i = 0; i<=9; i++) board[i]=OUTER;
    for (i = 10; i<=89; i++) {
        if (i%10 >= 1 && i%10 <= 8) board[i]=EMPTY; else board[i]=OUTER;
    }
    for (i = 90; i<=99; i++) board[i]=OUTER;
    board[44]=WHITE; board[45]=BLACK; board[54]=BLACK; board[55]=WHITE;
}

/*
	Called at the start of execution on all ranks except for rank 0.
	This is where messages are passed between workers to guide the search.
 */
void run_worker(int rank){
    int recv_loc, loc_score, depth, temp_score;
    double elapsed_time;
    board = (int *)malloc(BOARDSIZE * sizeof(int));
    //Syncing of initialised board
    sync_board();
    while(running == 1){
        sync_board();
        if(running == 0){
            break;
        }
        depth = 4;
        start_time = MPI_Wtime();
        elapsed_time = 0;
        request_move();
        recv_loc = retrieve_move(rank);
        loc_score = 0;
        if(recv_loc != -1){
            while(elapsed_time <= (time_limit - TIME_OFFSET) && depth < MAX_DEPTH){
                temp_score = run_minimax(recv_loc, depth,INT_MIN, INT_MAX, 1);
                depth++;
                elapsed_time = MPI_Wtime() - start_time;
                if(elapsed_time < (time_limit - TIME_OFFSET) && temp_score != INT_MIN && temp_score != INT_MAX){
                    loc_score = temp_score;
                }
                
            }
        }
        MPI_Send(&recv_loc, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&loc_score, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        
    }
}

/*
*   Retrieves the move to play based on rank of the process.
    Param:
        rank - rank of the process;
    Return:
        loc - location of move to play.
*/
int retrieve_move(int rank){
    
    int loc;
    int *moves;
    if(my_colour == EMPTY){
        my_colour = BLACK;
    }
    moves = legalmoves(my_colour);
    if (moves[0] == 0){
        loc = -1;
    }else{
        if(rank <= moves[0]){
            loc = moves[rank];
        }else{
            srand ((unsigned) rank);
            loc = moves[(rand() % moves[0]) + 1];
        }
    }
    free(moves);
    return loc;
}

/*
	Called when the other engine has made a move. The move is given in a
	string parameter of the form "xy", where x and y represent the row
	and column where the opponent's piece is placed, respectively.
 */
void play_move(char *move){
    int loc;
    if (my_colour == EMPTY){
        my_colour = WHITE;
    }
    if (strcmp(move, "pass") == 0){
        return;
    }
    loc = get_loc(move);
    makemove(loc, opponent(my_colour));
}

/*
	Called when the match is over.
 */
void game_over(){
    MPI_Finalize();
}

char* get_move_string(int loc){
    static char ms[3];
    int row, col, new_loc;
    new_loc = loc - (9 + 2 * (loc / 10));
    row = new_loc / 8;
    col = new_loc % 8;
    ms[0] = row + '0';
    ms[1] = col + '0';
    ms[2] = '\n';
    return ms;
}

int get_loc(char* movestring){
    int row, col;
    row = movestring[0] - '0';
    col = movestring[1] - '0';
    return (10 * (row + 1)) + col + 1;
}

int *legalmoves (int player) {
    int move, i, *moves;
    moves = (int *)malloc(65 * sizeof(int));
    moves[0] = 0;
    i = 0;
    for (move=11; move<=88; move++)
        if (legalp(move, player)) {
            i++;
            moves[i]=move;
        }
    moves[0]=i;
    return moves;
}

int legalp (int move, int player) {
    int i;
    if (!validp(move)) return 0;
    if (board[move]==EMPTY) {
        i=0;
        while (i<=7 && !wouldflip(move, ALLDIRECTIONS[i], player)) i++;
        if (i==8) return 0; else return 1;
    }
    else return 0;
}

int validp (int move) {
    if ((move >= 11) && (move <= 88) && (move%10 >= 1) && (move%10 <= 8))
        return 1;
    else return 0;
}

int wouldflip (int move, int dir, int player) {
    int c;
    c = move + dir;
    if (board[c] == opponent(player))
        return findbracketingpiece(c+dir, dir, player);
    else return 0;
}

int findbracketingpiece(int square, int dir, int player) {
    while (board[square] == opponent(player)) square = square + dir;
    if (board[square] == player) return square;
    else return 0;
}

int opponent (int player) {
    switch (player) {
        case 1: return 2;
        case 2: return 1;
        default: printf("illegal player\n"); return 0;
    }
}

void makemove (int move, int player) {
    int i;
    board[move] = player;
    for (i=0; i<=7; i++) makeflips(move, ALLDIRECTIONS[i], player);
}

void makeflips (int move, int dir, int player) {
    int bracketer, c;
    bracketer = wouldflip(move, dir, player);
    if (bracketer) {
        c = move + dir;
        do {
            board[c] = player;
            c = c + dir;
        } while (c != bracketer);
    }
}

void printboard(){
    int row, col;
    fprintf(fp,"   1 2 3 4 5 6 7 8 [%c=%d %c=%d]\n",
            nameof(BLACK), count(BLACK, board), nameof(WHITE), count(WHITE, board));
    for (row=1; row<=8; row++) {
        fprintf(fp,"%d  ", row);
        for (col=1; col<=8; col++)
            fprintf(fp,"%c ", nameof(board[col + (10 * row)]));
        fprintf(fp,"\n");
    }
    fflush(fp);
}



char nameof (int piece) {
    static char piecenames[5] = ".bw?";
    return(piecenames[piece]);
}

int count (int player, int * board) {
    int i, cnt;
    cnt=0;
    for (i=1; i<=88; i++)
        if (board[i] == player) cnt++;
    return cnt;
}
/*
    Improved Evaluation function.
    Loops over the board and adds points to the specific player depending where
    they played.
    Points Distrubution:
        - 1 Point for a normal inner board space.
        - 5 Points for an edge board space.
        - 10 Points for a corner board space.
*/
int evaluate_board(){
    int my_score,opp_score, i;
    my_score = 0;
    opp_score = 0;

    // Points are added as 11 and 6 to also take into account the point for having
    // a piece on the board.
    for (i = 10; i<=89; i++) {
        if(i == 11 || i == 18 || i == 81 || i == 88){
            if(board[i] == my_colour){
                my_score = my_score + 11;
            }else if(board[i] == opponent(my_colour)){
                opp_score = opp_score + 11;
            }
        }else if (i%10 == 1 || i%10 == 8){ 
            if(board[i] == my_colour){
                my_score = my_score + 6;
            }else if(board[i] == opponent(my_colour)){
                opp_score = opp_score + 6;
            }
        }else if((i > 11 && i < 18) || (i > 81 && i < 88)){
            if(board[i] == my_colour){
                my_score = my_score + 6;
            }else if(board[i] == opponent(my_colour)){
                opp_score = opp_score + 6;
            }
        }else{
            if(board[i] == my_colour){
                my_score++;
            }else if(board[i] == opponent(my_colour)){
                opp_score++;
            }
        }
    }
    return (my_score-opp_score);
}

/*
*   Method that syncs the board across process, if the game is still running (If the game has terminated or not so that the all process can terminate)
*   and colour in play.
*/
void sync_board(){
    MPI_Bcast(&running, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(board, BOARDSIZE, MPI_INT,0,MPI_COMM_WORLD);
    MPI_Bcast(&my_colour, 1, MPI_INT, 0, MPI_COMM_WORLD);
}

/*
*   Method that allows the master process to request move from all other process.
*/
void request_move(){
    MPI_Bcast(&REQUEST_MOVE, 1, MPI_INT,0,MPI_COMM_WORLD);
}

/*
    Runs the recursive minimax function with alpha beta pruning.
    Params:
        loc - move that is being played.
        depth - depth of the search.
        alpha - alpha value used in alpha beta pruning
        beta - beta value used in alpha beta pruning
        maxPlayer - where the algorithm is trying to maximise or minimise score
    Returns:
        Result - Score based on the evalaution function and minimax algorithm.
*/
int run_minimax(int loc, int depth, int alpha, int beta, int maxPlayer){
    int eval, maxEval, minEval, i, result;
    int *childMoves, *prev_board;
    double elapsed_time;

    if(maxPlayer){
        makemove(loc, my_colour);
    }else{
        makemove(loc, opponent(my_colour));
    }

    elapsed_time = MPI_Wtime() - start_time;
    
    if (depth == 0 || loc == -1 || elapsed_time >= (time_limit - TIME_OFFSET)){
        result = evaluate_board();
        return result;
    }

    if(maxPlayer){
        maxEval = INT_MIN;
        childMoves = legalmoves(opponent(my_colour));
        prev_board = (int *)malloc(BOARDSIZE * sizeof(int));
        prev_board = memcpy(prev_board, board, BOARDSIZE * sizeof(int));
        for(i = 1; i <= childMoves[0]; i++){
            eval = run_minimax(childMoves[i], depth-1, alpha,  beta, 0);
            maxEval = max(eval, maxEval);
            alpha = max(alpha, eval);
            if(beta <= alpha){
                break;
            }
            board = memcpy(board, prev_board, BOARDSIZE * sizeof(int));;
        }
        free(childMoves);
        free(prev_board);
        return maxEval;

    }else{
        minEval = INT_MAX;
        childMoves = legalmoves(my_colour);
        prev_board = (int *)malloc(BOARDSIZE * sizeof(int));
        prev_board = memcpy(prev_board, board, BOARDSIZE * sizeof(int));
        for(i = 1; i <= childMoves[0]; i++){
            eval = run_minimax(childMoves[i], depth-1, alpha,  beta, 1);
            minEval = min(eval, minEval);
            beta = min(beta, eval);
            if(alpha <= beta){
                break;
            }
            board = memcpy(board, prev_board, BOARDSIZE * sizeof(int));;
            
        }
        free(childMoves);
        free(prev_board);
        return minEval;
    }

    return -1;
}

/*
   Generic function to return the max between two numbers.
    Param:
        a - First Number to be compared.
        b - Second Number to be compared.
    Return:
        Max value between a and b.
*/
int max(int a, int b){
    if(a >= b){
        return a;
    } else{
        return b;
    }
}

/*
   Generic function to return the min between two numbers.
    Param:
        a - First Number to be compared.
        b - Second Number to be compared.
    Return:
        Min value between a and b.
*/
int min(int a, int b){
    if(a <= b){
        return a;
    } else{
        return b;
    }
}



