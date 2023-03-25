// The program uses a modified Warnsdorff's Heuristic based Backtracking algorithm.
// The backtracking is parallelised using pthreads and mutex locks.
// A new thread is dynamically created if the current thread count is less than a predefined max thread count.
// A recursive call is made otherwise.

// Group 52
// Group Member Details:
// 1. Raj Tripathi         2019B4A70869H f20190869@hyderabad.bits-pilani.ac.in
// 2. Satvik Omar          2019B4A70933H f20190933@hyderabad.bits-pilani.ac.in
// 3. Pranavi Gunda        2019B4A71068H f20191068@hyderabad.bits-pilani.ac.in
// 4. Pranjal Jasani       2019B4A70831H f20190831@hyderabad.bits-pilani.ac.in
// 5. Suraj Retheesh Nair  2020A7PS0051H f20200051@hyderabad.bits-pilani.ac.in



// A board[N][N] is maintained which keeps a track of the traversal.
// If the tour is found, the board is sent to be printed and the program execution stops.

// The heuristic is to choose the move from which the knight will have the fewest onward moves.
// In case there is a tie among choices, a tie-breaker is applied.
// The move having the max Euclidean Distance from the centre of the board is chosen. 



/***********************************************The code starts here******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h> // For pow(), sqrt()

//optional
#include <sys/types.h>
#include <sys/wait.h>

// Set the maximum number of threads you wish to run at a time
#define maxTCount 8


// Define a struct for a coordinate(x,y)
typedef struct{
	int x,y;
} pair;

// Define a struct to be used by pthread_create() to call the function with parameters
typedef struct{
	int move;
	pair pos;
	int *board;
	int isThread;
	pair *next;
	int *adjacentWeights;
} params;

int N;  // Size of board NxN
int control = 0; // Flag to stop execution
int threadCount = 1; // Keeps the current threadCount, initialised to 1(main thread)
pthread_mutex_t lock; // Recursive lock for function calls
pthread_mutex_t cLock; // Normal lock for printing output
pthread_mutex_t lock2; // Normal lock for printing output
pthread_mutexattr_t recurse; // To set attribute for <lock>

// To store information of the created threads in the calling thread
typedef struct  {
	pthread_t id;
	int valid;
} pthread_info;

// Function to check if the given move next is valid from current position s
int isValid(pair next, pair s,int *board)  {
	pair move = {s.x+next.x, s.y+next.y};

	return(move.x>=0 && move.x<N && move.y>=0 && move.y <N && *(board+move.x+N*move.y)==0);

}

// Utility Function for the heuristic
void swap(int* xp, int* yp)
{
    int temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// Utility Function for tie-breaking in the heuristic
float euclid(pair s)  {
	pair center = {N/2,N/2};
	return sqrt(pow(s.x-center.x, 2) + pow(s.y-center.y, 2)*1.0);
}

// Function to calculate weights for the heuristic
void *calcAdjacentWeights(void *p)  {
	params args = *((params*) p);
	int i = args.move;
	pair s = args.pos;

	int end = i+8;
	for(;i<end;i++)  {
		int count = 0;
		if(isValid(args.next[i], s, args.board))  {
			pair temp = {s.x+args.next[i].x, s.y+args.next[i].y};
			for(int j = 0; j<8; j++)  {
				if(isValid(args.next[j], temp, args.board))  {
					count++;
				}

			}
			args.adjacentWeights[i] = count;

		}
		else  {
			args.adjacentWeights[i] = 9;
		}
	}

}

// Function to sort the choices by priority, as defined by the heuristic.
void bubbleSort(int arr[], pair arr2[], int n, pair s)
{
    int i, j;
    for (i = 0; i < n - 1; i++)  {
        for (j = 0; j < n - i - 1; j++)  {
			pair pos1 = {s.x+arr2[j].x ,s.y+arr2[j].y};
			pair pos2 = {s.x+arr2[j+1].x ,s.y+arr2[j+1].y};
			//swap when bigger weight appears
				if ((arr[j] > arr[j + 1]))  {
		  		//if ((arr[j] > arr[j + 1]) || (arr[j] == arr[j + 1]  && euclid(pos2)>euclid(pos1)))  {
					swap(&arr[j], &arr[j + 1]);
					swap(&(arr2[j].x), &(arr2[j + 1]).x);
					swap(&(arr2[j].y), &(arr2[j + 1]).y);
			}
		}
	}
}

// Prints the tour and terminates the program
void print_path(int *board){
	if(control==0)  {
		++control;
		pair *path;
		path = malloc(N*N*sizeof(pair));

		for(int i = 0; i<N*N; i++)  {
			int x = i%N;
			int y = i/N;
			(*(path+ *(board+i) - 1)).x = x;
			(*(path+ *(board+i) - 1)).y = y;
		}

		for(int i=0;i<N*N;i++){
			printf("%d,%d|",(*(path+i)).x, (*(path+i)).y);			
		}
	}	
}

// The backbone of the program. Recursively searches for the tour(Creates new threads if possible).
void *marker(void *p)  {

	int n = N;
	params args = *((params*)p);
		if(control==1)  {
		pthread_exit(NULL);
	}
	int move = args.move;
	pair s = args.pos;
	int *board;
	pair valid[8];
	int adjacentWeights[8];
	pthread_info tid[8]; // to store the ids of newly created threads, if any
	pair next[8]={{2,1},{1,2},{-1,-2},{1,-2},{2,-1},{-1,2},{-2,1},{-2,-1}};

	// initialise the board and copy contents of the previous call
	 if(args.isThread==1)  {
	board = (int*) malloc((n)*(n)*sizeof(int));
		for(int i = 0; i<N*N; i++)  {
			*(board+i) = *(args.board+i);
		}
	}
	else{
		board = args.board;
	}
	
	// mark the move number for the given call
	*(board+s.x + N*s.y) = move;


	// Initialise the array for valid moves with -1,-1
	for(int i = 0; i<8; i++) {
		valid[i].x = -1;
		valid[i].y = -1;
		tid[i].valid=0;
		tid[i].id = 0;
	}
	
	// calculate weights for the heuristic
	params q = {.move=0, .pos=s, .adjacentWeights=adjacentWeights,.next=next, .board = board};
	calcAdjacentWeights(&q);

	// apply the heuristic and order choices
	bubbleSort(adjacentWeights, next, 8, s);

	// prepare the valid next possible moves
	int validCount = 0;
	for(int i = 0; i<8; i++)  {
		if(isValid(next[i],s,board))  {
			//success
			validCount++;
			valid[i].x = s.x+next[i].x;
			valid[i].y = s.y+next[i].y;
		}
	}
		
	if(move==n*n)  {
		//found a path
		pthread_mutex_lock(&cLock);
		print_path(board);
		pthread_mutex_unlock(&cLock);
		
	}
	else if(validCount == 0 && move!=n*n)  {

		// Do nothing, let the algorithm backtrack
	}

	else  {

		// for each valid move, check if a thread can be created(synchronized check).
		// create a thread if possible, otherwise call the function recursively with same arguments
		for(int i = 0; i<8; i++)  {
			tid[i].id = 0;	// to distinguish from actual ids(thread ids are always positive)
			if(valid[i].x != -1 && valid[i].y != -1)  {

				pthread_mutex_lock(&lock);
				if(threadCount<maxTCount)  {
					params q = {.move=move+1, .pos=valid[i], .board = board, .isThread=1}; 
					int isSuccessful = pthread_create(&tid[i].id, NULL, marker, (void *)(&q));
					if(isSuccessful!=0)  {
						tid[i].id = 0;
						i--;
						pthread_mutex_unlock(&lock);
					}
					else  {
					//if the thread was created successfully
					tid[i].valid = 1;
					threadCount++; //update the thread count
					pthread_mutex_unlock(&lock);
					}

				}
				else  {
					pthread_mutex_unlock(&lock);
					params r = {.move=move+1, .pos=valid[i], .board = board, .isThread=0}; 
					pthread_t tq = pthread_self();
					marker((void *)(&r));
					*(board+r.pos.x+n*r.pos.y)=0;
				}
			}
		}
	}


	// Wait for any threads created by the current function call
	for(int i = 0; i<8; i++)  {
		if(tid[i].valid==1)  {
			pthread_join(tid[i].id, NULL);
		}
	}

	// If the function call was thread, then update thread count and exit
	if(args.isThread==1)  {
		pthread_mutex_lock(&lock2);
		threadCount--;
		pthread_mutex_unlock(&lock2);
		pthread_exit(NULL);
	}
	return NULL;
}

// Function to initialise everything and call the marker function
void knightstour(int n, pair s, int move)  {

	// Initialise board to 0
	int *board;
	board = (int*) calloc((n)*(n),sizeof(int));

	int adjacentWeights[8];
	pair next[8]={{2,1},{1,2},{-1,-2},{1,-2},{2,-1},{-1,2},{-2,1},{-2,-1}};

	// Initialise starting position
	*(board+s.x + N*s.y) = move;


	// Initialise the array for valid moves with -1,-1
	pair valid[8];
	for(int i = 0; i<8; i++) {
		valid[i].x = -1;
		valid[i].y = -1;
	}

	// Calculate weights
	params q = {.move=0, .pos=s, .adjacentWeights=adjacentWeights, .next=next, .board = board};
	calcAdjacentWeights(&q);



	// Sort using the weights
	bubbleSort(adjacentWeights, next, 8, s);



	// Count the number of valid moves
	int validCount = 0;
	for(int i = 0; i<8; i++)  {
		if(isValid(next[i], s, board))  {
			//success
			validCount++;
			valid[i].x = s.x+next[i].x;
			valid[i].y = s.y+next[i].y;
		}
	}

	if(validCount == 0)  {
		// If the input was 1 0 0
		printf("0,0|");
	}
	else  {

		// For each valid move, call the marker function
		for(int i = 0; i<8; i++)  {
			if(valid[i].x != -1 && valid[i].y != -1)  {
				params p = {.move=move+1, .pos=valid[i], .isThread=0, .board=board}; 
				marker((void*)(&p));
				*(board+p.pos.x+n*p.pos.y)=0;
			}
		}	
	}
}

// For running the program for the given input
void run(int N, pair s);

//for Testing the program for all inputs from 5 to N
void testMax(int N);


// A Knight's tour with a fixed s does not exist iff ((N is odd and X+Y is odd) or 2<=N<=4)
// Since at every Knight's move, the sum (X+Y) alternates between odd and even
// An odd board has more even squares, so the Knight must start at an even square

int main(int argc, char *argv[]) {
	// If the command line input was invalid
	if (argc != 4) {
		printf("Usage: ./Knight.out grid_size StartX StartY");
		exit(-1);
	}
	
	// Read the inputs
	N = atoi(argv[1]);
	int StartX=atoi(argv[2]);
	int StartY=atoi(argv[3]);

	//Check if the input was valid
	if(N<=0 || StartX<0 || StartX>=N || StartY<0 || StartY>=N)  {
		printf("Please Enter a Valid Input!");
		return 0;
	}
	// No possible tour
	if((N%2 != 0 && (StartX+StartY)%2 != 0) || (N == 2 || N == 3 || N == 4))  {		
		printf("No Possible Tour");		
	}

	// A tour exists
	else  {
		pair s,t;
		s.x = StartX;
		s.y = StartY;
		// to initialise the recursive lock declare above
		pthread_mutexattr_init(&recurse);
		pthread_mutexattr_settype(&recurse, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&lock, &recurse);
		
		// Find and print the tour
		knightstour(N,s,1);
		//run(N,s);

	return 0;
	}
}


void run(int N, pair s)  {
	int status;
	control=0;
	pid_t id = fork();
	if(id==0)  {
		knightstour(N, s, 1);
	}
	wait(&status);
	if(status!=0)
		printf("Something went wrong: Error signal %d",status);
}


