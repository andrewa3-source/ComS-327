#include <stdio.h>
#include <stdlib.h>
#include <time.h>


//char map[81][21];
typedef struct map{
    char map[81][21];
    int n;
    int s;
    int e;
    int w;

} map_t;

// typedef struct entire{
//     map_t *ent[401][401];
// }entire_t;



//initialize the map by placing the surrounding border and filling it with grass
int init(map_t *mr, int x_loco, int y_loco){
    map_t m;
    int i, j;
    for(i=0; i<81; ++i){
        for(j=0; j<21; ++j){
            if(i == 80){
                 m.map[i][j] = '\n';
            }
            else if(i == 0 || j == 0 || j == 20 || i == 79){
                m.map[i][j] = '%';
            }
            else{
                m.map[i][j] = '.';
            }
        }
    }
    mr = &m;
    return 1;
}

int roads(map_t *m[401][401], int x_loco, int y_loco){
    int i;
    int top;
    int bot;
    int left;
    int right;

    if (x_loco != 400 || x_loco != 0 || y_loco != 0 || y_loco != 400){
        //choose exit locations for N,E,S,W
        //check if the south map has a N gate
        printf("%d\n",m[x_loco][y_loco + 1]->n);
        if(m[x_loco][y_loco + 1]->n != 0){
            bot = m[x_loco][y_loco + 1]->n;
            //printf("yo\n");
        }else{
            bot = rand() % 78 + 1;
        }
        

        //check if the east map has a west gate
        if(m[x_loco + 1][y_loco]->w != 0){
            right = m[x_loco + 1][y_loco]->w;
        }else{
            right = rand() % 19 + 1;
        }
        

        //check if the north map has a south gate
        if(m[x_loco][y_loco - 1]->s != 0){
            top = m[x_loco][y_loco - 1]->s;
            printf("%d\n", m[x_loco][y_loco - 1]->s);

        }else{
            top = rand() % 78 + 1;
        }
        

        //check if the west map has a east gate
        if(m[x_loco - 1][y_loco]->e != 0){
            left = m[x_loco - 1][y_loco - 1]->e;
        }else{
            left = rand() % 19 + 1;
        }
        


        m[x_loco][y_loco]->map[top][0] = '#';
        m[x_loco][y_loco]->map[bot][20] = '#';
        m[x_loco][y_loco]->map[0][left] = '#';
        m[x_loco][y_loco]->map[79][right] = '#';
        //printf("%d %d %d %d\n", top, bot, right, left);

        m[x_loco][y_loco]->s = bot;
        m[x_loco][y_loco]->e = right;
        m[x_loco][y_loco]->n = top;
        m[x_loco][y_loco]->w = left;

        //printf("%d %d %d %d\n",  m->ent[x_loco][y_loco]->n, m->ent[x_loco][y_loco]->s, m->ent[x_loco][y_loco]->e, m->ent[x_loco][y_loco]->w);
        //connecting left and right
        int dis;
        int dist = rand() % 75 + 2;
        for(i = 0; i < dist; ++i){
            m[x_loco][y_loco]->map[i][left] = '#';
        }
        
        int dif = left - right;
        if(dif >= 0){
            for(i = left; i >= right; --i){
                m[x_loco][y_loco]->map[dist - 1][i] = '#';
            }
        }
        else{
            for(int i = left; i <= right; ++i){
                m[x_loco][y_loco]->map[dist - 1][i] = '#';
            }
        }
        for(i = dist; i < 80; ++i){
            m[x_loco][y_loco]->map[i][right] = '#';
        }


        //connecting top and bottem
        dis = rand() % 17 + 2;
        for(i = 0; i < dis; ++i){
            m[x_loco][y_loco]->map[top][i] = '#';
        }
        dif = top - bot;
        if(dif >= 0){
            for(i = bot; i <= top; ++i){
                m[x_loco][y_loco]->map[i][dis] = '#';
            }
        }
        else{
            for(i = top; i <= bot; ++i){
                m[x_loco][y_loco]->map[i][dis] = '#';
            }
        }
        for(i = dis; i < 21; ++i){
            m[x_loco][y_loco]->map[bot][i] = '#';
        }
    }


    

    // //place poke marts and centers
    // if(map[dist/2][left-1] != '#' && map[dist/2][left-1] != '%' &&
    //  map[dist/2][left-2] != '#' && map[dist/2][left-2] != '%' && map[(dist/2) + 1][left-1] != '#' && map[(dist/2) + 1][left-1] != '%' 
    //  && map[(dist/2) + 1][left-2] != '#' && map[(dist/2) + 1][left-2] != '%'){
    //     map[dist/2][left-1] = 'C';
    //     map[dist/2][left-2] = 'C';
    //     map[(dist/2) + 1][left-1] = 'C';
    //     map[(dist/2) + 1][left-2] = 'C';
    // }
    // else{
    //     map[76][right-1] = 'C';
    //     map[76][right-2] = 'C';
    //     map[75][right-1] = 'C';
    //     map[75][right-2] = 'C';
    // }

    // if(map[dist/2][left+1] != '#' && map[dist/2][left+1] != '%' &&
    //  map[dist/2][left+2] != '#' && map[dist/2][left+2] != '%' && map[(dist/2) + 1][left+1] != '#' && map[(dist/2) + 1][left+1] != '%' 
    //  && map[(dist/2) + 1][left+2] != '#' && map[(dist/2) + 1][left+2] != '%'){
    //     map[dist/2][left+1] = 'M';
    //     map[dist/2][left+2] = 'M';
    //     map[(dist/2) + 1][left+1] = 'M';
    //     map[(dist/2) + 1][left+2] = 'M';
    // }
    // else{
    //     map[76][right+1] = 'M';
    //     map[76][right+2] = 'M';
    //     map[75][right+1] = 'M';
    //     map[75][right+2] = 'M';
    // }
    
    return 1;
}

 int print_map(map_t *m, int x_loco, int y_loco){
    int i, j;
    for(j=0; j<21; ++j){
        for(i=0; i<81; ++i){
            printf("%c", m->map[i][j]);
        }
    }
    return 1;
}


// int tall_grass(){
//     int i, j;
//     int x, y;
//     int xsize = 14;
//     int ysize = 6;
//     int found = 0;
//     while(!found){
//         found = 1;
//         x = rand() % 65 + 3;
//         y = rand() % 12 + 2;

//         for(i = x; i <= x + xsize; ++i){
//             for(j = y; j < y + ysize; ++j){
//                 if(map[i][j] == '%' || map[i][j] == ':' || map[i][j] == '~'){
//                     found = 0;
//                 }
//             }
//         }
//     }

//     for(i = x; i < x+xsize; ++i){
//         for(j = y; j < y+ysize; ++j){
//             map[i][j] = ':';
//         }
//     }
    
//     return 1;
// }


// int water(){
//     int i, j;
//     int x, y;
//     int xsize = 12;
//     int ysize = 5;
//     int found = 0;
//     while(!found){
//         found = 1;
//         x = rand() % 65 + 3;
//         y = rand() % 12 + 2;

//         for(i = x; i <= x + xsize; ++i){
//             for(j = y; j < y + ysize; ++j){
//                 if(map[i][j] == '%' || map[i][j] == ':' || map[i][j] == '~'){
//                     found = 0;
//                 }
//             }
//         }
//     }

//     for(i = x; i < x+xsize; ++i){
//         for(j = y; j < y+ysize; ++j){
//             map[i][j] = '~';
//         }
//     }
    
//     return 1;
// }

// int boulder(){
//     int i, j;
//     int x, y;
//     int xsize = 15;
//     int ysize = 12;
//     int found = 0;
//     while(!found){
//         found = 1;
//         x = rand() % 65 + 3;
//         y = rand() % 12 + 2;

//         for(i = x; i <= x + xsize; ++i){
//             for(j = y; j < y + ysize; ++j){
//                 if(map[i][j] == '%' || map[i][j] == ':' || map[i][j] == '~'){
//                     found = 0;
//                 }
//             }
//         }
//     }

//     for(i = x; i < x+xsize; ++i){
//         for(j = y; j < y+ysize; ++j){
//             map[i][j] = '%';
//         }
//     }
    
//     return 1;
// }

// int trees(){
//     int i, j;
//     int x, y;
//     int xsize = 4;
//     int ysize = 2;
//     int found = 0;
//     while(!found){
//         found = 1;
//         x = rand() % 65 + 3;
//         y = rand() % 12 + 2;

//         for(i = x; i <= x + xsize; ++i){
//             for(j = y; j < y + ysize; ++j){
//                 if(map[i][j] == '%' || map[i][j] == ':' || map[i][j] == '~'){
//                     found = 0;
//                 }
//             }
//         }
//     }

//     for(i = x; i < x+xsize; ++i){
//         for(j = y; j < y+ysize; ++j){
//             map[i][j] = '"';
//         }
//     }
    
//     return 1;
// }

int main(int argc, char *argv[]){
    srand(time(NULL));
    //entire_t m;
    map_t *ent[401][401];
    int in_game = 1;
    char c;
    int x_loco = 200;
    int y_loco = 200;
    //Initialize all maps
    for(int i = 0; i < 400; ++i){
        for(int j = 0; j < 400; ++j){
        //    map_t r = { 0 };
            // r.s=-1;
            // r.n=-1;
            // r.e=-1;
            // r.w=-1;
            

            ent[i][j] = NULL;
            // ent[i][j]->s = -1;
            // ent[i][j]->n = -1;
            // ent[i][j]->w = -1;
            // ent[i][j]->e = -1;
        }
    }

    init(ent[x_loco][y_loco], x_loco, y_loco);
    //roads(ent, x_loco, y_loco);
    print_map(ent[x_loco][y_loco], x_loco, y_loco);
    //print_map(ent, 100, 100);
    
    while(in_game){
        printf("Enter a command: ");
        scanf(" %c", &c);
        switch(c){
        case 'n':
            y_loco -= 1;
            printf("%d\n", y_loco);
            printf("%d\n", ent[x_loco][y_loco]->s);
            //init(ent, x_loco, y_loco);
            //roads(ent, x_loco, y_loco);
            //print_map(ent, x_loco, y_loco);
            break;
        
        case 's':
            //print_map(ent, x_loco, y_loco);
            break;
        
        case 'e':
            break;

        case 'w':
            break;

        case 'q':
            in_game = 0;
            break;
            
        
        case 'f':
            scanf(" %d %d", &x_loco, &y_loco);
            //init(ent, x_loco, y_loco);
            roads(ent, x_loco, y_loco);
            //print_map(ent, x_loco, y_loco);
            break;

        default:
            break;

        
    }
        

    }
    

    


}