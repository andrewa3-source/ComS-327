#include <stdio.h>
#include <stdlib.h>
#include <time.h>

char map[81][21];

typedef struct map{
    char map[80][21];
    int n;
    int s;
    int e;
    int w;

} map_t;


typedef struct entire {
  map_t *entire[400][400];
  
  
} entire_t;



typedef struct zoneloco{
    int x_tg1;
    int y_tg1;

    int x_tg2;
    int y_tg2;

    int x_b;
    int y_b;

    int x_w;
    int y_w;

    int x_sg;
    int y_sg;

    int x_tr;
    int y_tr;

    char gmap[80][21];
}zoneloco_t;



int growth(zoneloco_t *z){
    int i;
    int j;

    for(i=0; i<81; ++i){
        for(j=0; j<21; ++j){
            if(i == 0 || j == 0 || j == 20 || i == 79){
                z->gmap[i][j] = '%';
            }
            else if(i == 80){
                
            }
            else{
                z->gmap[i][j] = '.';
            }
        }
    }

    z->x_tg1 = rand() % 18 + 3;
    z->y_tg1 = rand() % 6 + 3;

    z->gmap[z->x_tg1][z->y_tg1] = ':';

    z->x_tg2 = rand() % 18 + 50;
    z->y_tg2 = rand() % 6 + 12;

    z->gmap[z->x_tg2][z->y_tg2] = ':';

    z->x_b = rand() % 18 + 30;
    z->y_b = rand() % 6 + 5;

    z->gmap[z->x_b][z->y_b] = '%';

    z->x_sg = rand() % 18 + 20;
    z->y_sg = rand() % 10 + 8;

    z->gmap[z->x_sg][z->y_sg] = '.';

    z->x_w = rand() % 18 + 60;
    z->y_w = rand() % 6 + 3;

    z->gmap[z->x_w][z->y_w] = '~';

    z->x_tr = rand() % 18 ;
    z->y_tr = rand() % 6 + 10;

    z->gmap[z->x_tr][z->y_tr] = '"';


    int tg1_c = 0;
    int tg2_c = 0;
    int b_c = 0;
    //int sg_c = 0;
    int tr_c = 0;
    int w_c = 0;


    while (!tg1_c || !tg2_c || !b_c || !tr_c || !w_c){
        for(i = 0; i < 80; i++){
            for(j = 0; j < 21; ++j){
                
                if (z->gmap[i][j] == ':'){
                    
                }




            }
        }

    }
    return 1;
}


//initialize the map by placing the surrounding border and filling it with grass
int init(){
    int i, j;
    for(i=0; i<81; ++i){
        for(j=0; j<21; ++j){
            if(i == 80){
                 map[i][j] = '\n';
            }
            else if(i == 0 || j == 0 || j == 20 || i == 79){
                map[i][j] = '%';
            }
            else{
                map[i][j] = '.';
            }
        }
    }
    return 1;
}

int roads(){
    int i;
    //choose exit locations for N,E,S,W
    int left = rand() % 15 + 3;
    map[0][left] = '#';
    int right = rand() % 18 + 2;
    map[79][right] = '#';
    int top = rand() % 77 + 2;
    map[top][0] = '#';
    int bot = rand() % 77 + 2;
    map[bot][20] = '#';

    //connecting left and right
    int dis;
    int dist = rand() % 75 + 2;
    for(i = 0; i < dist; ++i){
        map[i][left] = '#';
    }
    
    int dif = left - right;
    if(dif >= 0){
        for(i = left; i >= right; --i){
            map[dist - 1][i] = '#';
        }
    }
    else{
        for(int i = left; i <= right; ++i){
            map[dist - 1][i] = '#';
        }
    }
    for(i = dist; i < 80; ++i){
        map[i][right] = '#';
    }


    //connecting top and bottem
    dis = rand() % 17 + 2;
    for(i = 0; i < dis; ++i){
        map[top][i] = '#';
    }
    dif = top - bot;
    if(dif >= 0){
        for(i = bot; i <= top; ++i){
            map[i][dis] = '#';
        }
    }
    else{
        for(i = top; i <= bot; ++i){
            map[i][dis] = '#';
        }
    }
    for(i = dis; i < 21; ++i){
        map[bot][i] = '#';
    }

    //place poke marts and centers
    if(map[dist/2][left-1] != '#' && map[dist/2][left-1] != '%' &&
     map[dist/2][left-2] != '#' && map[dist/2][left-2] != '%' && map[(dist/2) + 1][left-1] != '#' && map[(dist/2) + 1][left-1] != '%' 
     && map[(dist/2) + 1][left-2] != '#' && map[(dist/2) + 1][left-2] != '%'){
        map[dist/2][left-1] = 'C';
        map[dist/2][left-2] = 'C';
        map[(dist/2) + 1][left-1] = 'C';
        map[(dist/2) + 1][left-2] = 'C';
    }
    else{
        map[76][right-1] = 'C';
        map[76][right-2] = 'C';
        map[75][right-1] = 'C';
        map[75][right-2] = 'C';
    }

    if(map[dist/2][left+1] != '#' && map[dist/2][left+1] != '%' &&
     map[dist/2][left+2] != '#' && map[dist/2][left+2] != '%' && map[(dist/2) + 1][left+1] != '#' && map[(dist/2) + 1][left+1] != '%' 
     && map[(dist/2) + 1][left+2] != '#' && map[(dist/2) + 1][left+2] != '%'){
        map[dist/2][left+1] = 'M';
        map[dist/2][left+2] = 'M';
        map[(dist/2) + 1][left+1] = 'M';
        map[(dist/2) + 1][left+2] = 'M';
    }
    else{
        map[76][right+1] = 'M';
        map[76][right+2] = 'M';
        map[75][right+1] = 'M';
        map[75][right+2] = 'M';
    }
    
    return 1;
}

int print_map(){
    int i, j;
    for(j=0; j<21; ++j){
        for(i=0; i<81; ++i){
            printf("%c", map[i][j]);
        }
    }
    return 1;
}


int tall_grass(){
    int i, j;
    int x, y;
    int xsize = 14;
    int ysize = 6;
    int found = 0;
    while(!found){
        found = 1;
        x = rand() % 65 + 3;
        y = rand() % 12 + 2;

        for(i = x; i <= x + xsize; ++i){
            for(j = y; j < y + ysize; ++j){
                if(map[i][j] == '%' || map[i][j] == ':' || map[i][j] == '~'){
                    found = 0;
                }
            }
        }
    }

    for(i = x; i < x+xsize; ++i){
        for(j = y; j < y+ysize; ++j){
            map[i][j] = ':';
        }
    }
    
    return 1;
}


int water(){
    int i, j;
    int x, y;
    int xsize = 12;
    int ysize = 5;
    int found = 0;
    while(!found){
        found = 1;
        x = rand() % 65 + 3;
        y = rand() % 12 + 2;

        for(i = x; i <= x + xsize; ++i){
            for(j = y; j < y + ysize; ++j){
                if(map[i][j] == '%' || map[i][j] == ':' || map[i][j] == '~'){
                    found = 0;
                }
            }
        }
    }

    for(i = x; i < x+xsize; ++i){
        for(j = y; j < y+ysize; ++j){
            map[i][j] = '~';
        }
    }
    
    return 1;
}

int boulder(){
    int i, j;
    int x, y;
    int xsize = 15;
    int ysize = 12;
    int found = 0;
    while(!found){
        found = 1;
        x = rand() % 65 + 3;
        y = rand() % 12 + 2;

        for(i = x; i <= x + xsize; ++i){
            for(j = y; j < y + ysize; ++j){
                if(map[i][j] == '%' || map[i][j] == ':' || map[i][j] == '~'){
                    found = 0;
                }
            }
        }
    }

    for(i = x; i < x+xsize; ++i){
        for(j = y; j < y+ysize; ++j){
            map[i][j] = '%';
        }
    }
    
    return 1;
}

int trees(){
    int i, j;
    int x, y;
    int xsize = 4;
    int ysize = 2;
    int found = 0;
    while(!found){
        found = 1;
        x = rand() % 65 + 3;
        y = rand() % 12 + 2;

        for(i = x; i <= x + xsize; ++i){
            for(j = y; j < y + ysize; ++j){
                if(map[i][j] == '%' || map[i][j] == ':' || map[i][j] == '~'){
                    found = 0;
                }
            }
        }
    }

    for(i = x; i < x+xsize; ++i){
        for(j = y; j < y+ysize; ++j){
            map[i][j] = '"';
        }
    }
    
    return 1;
}

int main(int argc, char *argv[]){
    srand(time(NULL));
    init();
    boulder();
    tall_grass();
    tall_grass();
    water();
    water();
    trees();
    trees();
    trees();
    trees();
    roads();
    print_map();
}