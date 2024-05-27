#include <stdio.h>
#define N 5

int num = 0;
//this will be code for assignment one in ComS 372
//author andrewa3
//date 01-19-23

//functions

//Determines whether the cell at the given position is a valid move for the knight
int avaliableSpace(int x, int y, int sol[N][N]){
    return (x >= 0 && x < N && y >= 0 && y < N && sol[x][y] == -1);
}

//used to print in array format
void printTour(int sol[N][N]){
    int i, j;
    int count = 1;
    int basic[N][N];

    for (i = 0; i < 5; ++i){
        for (j = 0; j < 5; ++j) {
            basic[i][j] = count;
            count++;
        }
    }

    count = 0;
    while (count < 25){

        for (i = 0; i < 5; ++i){
            for (j = 0; j < 5; ++j) {
                if (sol[i][j] == count && count != 24){
                    printf("%d,", basic[i][j]);
                    count++;
                }
                else if (sol[i][j] == count){
                    printf("%d", basic[i][j]);
                    count++;
                }
            }
        }

    }
    ++num;
}

//used to print in chessboard format
void printSolution(int sol[N][N]) 
{ 
    for (int x = 0; x < N; x++) { 
        for (int y = 0; y < N; y++) 
            printf(" %2d ", sol[x][y]); 
        printf("\n"); 
    } 
} 

//recursive function to be used by tour at
int tourUtil(int x, int y, int move, int xMove[], int yMove[], int sol[N][N]){
    //looping variable, next x pos, next y pos
    int k, nx, ny;

    //loop through all possible combinatins
    for (k = 0; k < 8; ++k) {

        if(move == N * N){
        //printf("\n");
        //printSolution(sol);
        //printf("\n");
        printTour(sol);
        printf("\n");
        sol[x][y] = -1;
        return 1;
        }


        nx = x + xMove[k];
        ny = y + yMove[k];

        if (avaliableSpace(nx, ny, sol)){
            
            sol[nx][ny] = move;
            int result = tourUtil(nx, ny, move + 1, xMove, yMove, sol);  
            
            if(result == 0) {
                sol[nx][ny] = -1;
            }
        }
    }
    return 0;
}

//find all the possible tours for a given starting space
int tour(){
    //used to find the next knight moves (L shapes)
    int xMove[8] = { 2, 1, -1, -2, -2, -1, 1, 2 }; 
    int yMove[8] = { 1, 2, 2, 1, -1, -2, -2, -1 };
    int i, j;
    int sol[N][N];
    int row = 0;
    int col = 0;

    while (row < 5 && col < 5){

        for(i = 0; i < 5; ++i){
            for(j = 0; j < 5; ++j){
               sol[i][j] = -1;
            }
        }
        sol[row][col] = 0;

        tourUtil(row, col, 1, xMove, yMove, sol);

        col++;
        
        if(col == 5){
            col = 0;
            row++;
        }

    }
    return 0;
}


int main(int argc, char *argv[]){
    tour();

    //print the total number of tours found
    //printf("\n %d \n", num);
    return 0;
}


