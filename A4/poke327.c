#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <endian.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/time.h>
#include <assert.h>
#include<math.h>

#include "heap.h"



#define malloc(size) ({          \
  void *_tmp;                    \
  assert((_tmp = malloc(size))); \
  _tmp;                          \
})

// int pc_x;
// int pc_y;

typedef struct path {
  heap_node_t *hn;
  uint8_t pos[2];
  uint8_t from[2];
  int32_t cost;
} path_t;

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef uint8_t pair_t[num_dims];

#define MAP_X              80
#define MAP_Y              21
#define MIN_TREES          10
#define MIN_BOULDERS       10
#define TREE_PROB          95
#define BOULDER_PROB       95

#define mappair(pair) (m->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (m->map[y][x])
#define mapxyz(x, y) (m[x_loco][y_loco]->map[y][x])
#define heightpair(pair) (m->height[pair[dim_y]][pair[dim_x]])
#define heightxy(x, y) (m->height[y][x])

int hiker_costs[] = {INT_MAX, INT_MAX, 10, 50, 50, 15, 10, 15, 15, INT_MAX, INT_MAX};
int rival_costs[] = {INT_MAX, INT_MAX, 10, 50, 50, 20, 10, INT_MAX, INT_MAX, INT_MAX, INT_MAX};

typedef enum __attribute__ ((__packed__)) terrain_type {
  ter_debug, //0
  ter_boulder, //1
  ter_tree, //2
  ter_path, //3
  ter_mart, //4
  ter_center, //5
  ter_grass, //6
  ter_clearing, //7
  ter_mountain, //8
  ter_forest, //9
  ter_water, //10
  player_pc //11
} terrain_type_t;

typedef struct map {
  terrain_type_t map[MAP_Y][MAP_X];
  uint8_t height[MAP_Y][MAP_X];
  uint8_t n, s, e, w;

  uint32_t hmap[MAP_Y][MAP_X];
  uint32_t rmap[MAP_Y][MAP_X];

  pair_t pc;
} map_t;

typedef struct queue_node {
  int x, y;
  struct queue_node *next;
} queue_node_t;

static int32_t path_cmp(const void *key, const void *with) {
  return ((path_t *) key)->cost - ((path_t *) with)->cost;
}

static int32_t edge_penalty(uint8_t x, uint8_t y)
{
  return (x == 1 || y == 1 || x == MAP_X - 2 || y == MAP_Y - 2) ? 2 : 1;
}

static void dijkstra_path(map_t *m, pair_t from, pair_t to){
  static path_t path[MAP_Y][MAP_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      path[y][x].cost = INT_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, path_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      path[y][x].hn = heap_insert(&h, &path[y][x]);
    }
  }

  while ((p = heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != from[dim_x]) || (y != from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        mapxy(x, y) = ter_path;
        heightxy(x, y) = 0;
      }
      heap_delete(&h);
      return;
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1)))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1));
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1)))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1));
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}


  


static void dijkstra_for_hiker(map_t *m)
{
  static path_t path[MAP_Y][MAP_X], *p;

  uint32_t initialized = 0;

  heap_t h;

  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  

  for (y = 0; y < MAP_Y; y++){
		for (x = 0; x < MAP_X; x++){
			path[y][x].cost = INT_MAX;
		}
	} 



  path[m->pc[dim_x]][m->pc[dim_y]].cost = 0;

  //printf("%d\n", m->pc[dim_y]);
  heap_init(&h, path_cmp, NULL);

  for (y = 0; y < MAP_Y; y++) {

    for (x = 0; x < MAP_X; x++) {
       if(mapxy(x, y) != ter_boulder && mapxy(x, y) != ter_tree && mapxy(x, y) != ter_water && mapxy(x, y)){

          path[y][x].hn = heap_insert(&h, &path[y][x]);

      }
      else{

        path[y][x].hn = NULL;

      }

    }

  }

  
  

  while ((p = heap_remove_min(&h))) {
      // m->hmap[p->pos[dim_y]][p->pos[dim_x]] = p->cost;
      p->hn = NULL;
      
      //printf("%d\n", p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1]]);
      //printf("%d %d", p->pos[dim_y], p->pos[dim_x]);
      

    //Top left
    if(( path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost > (p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1] - 1]))){
        // ------------------------------------------------------------------------------------
        path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost = p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1] - 1];
        //m->hmap[p->pos[dim_y] -1][p->pos[dim_x] - 1] =p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1]];
				path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn);
        // ------------------------------------------------------------------------------------
    }
    
    //printf("%d\n", hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1]]);
    //Top
    if(( path[p->pos[dim_y] - 1][p->pos[dim_x]].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost > (p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1] - 1]))){
		// ------------------------------------------------------------------------------------
			path[p->pos[dim_y] - 1][p->pos[dim_x]].cost = p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x]] - 1];
      //m->hmap[p->pos[dim_y] -1][p->pos[dim_x]] =p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x]]];
			path[p->pos[dim_y] - 1][p->pos[dim_x]].from[dim_y] = p->pos[dim_y];
			path[p->pos[dim_y] - 1][p->pos[dim_x]].from[dim_x] = p->pos[dim_x];
			heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x]].hn);
		//-------------------------------------------------------------------------------------
		}

    //Top Right
    if(( path[p->pos[dim_y] -1][p->pos[dim_x] + 1].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost > (p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] + 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost = p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] + 1] - 1];
        //m->hmap[p->pos[dim_y] -1][p->pos[dim_x] + 1] =p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] + 1]];
				path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn);
			//-------------------------------------------------------------------------------------
		}


    // Left
		if(( path[p->pos[dim_y]][p->pos[dim_x] - 1].hn) &&(path[p->pos[dim_y]][p->pos[dim_x] - 1].cost > (p->cost + hiker_costs[m->map[p->pos[dim_y]][p->pos[dim_x] - 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y]][p->pos[dim_x] - 1].cost = p->cost + hiker_costs[m->map[p->pos[dim_y]][p->pos[dim_x] - 1] - 1];
        //m->hmap[p->pos[dim_y]][p->pos[dim_x] - 1] =p->cost + hiker_costs[m->map[p->pos[dim_y]][p->pos[dim_x] - 1]];
				path[p->pos[dim_y]][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y]][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] - 1].hn);
			//-------------------------------------------------------------------------------------
		}

    // Right
		if(( path[p->pos[dim_y]][p->pos[dim_x] + 1].hn) &&(path[p->pos[dim_y]][p->pos[dim_x] + 1].cost > (p->cost + hiker_costs[m->map[p->pos[dim_y]][p->pos[dim_x] + 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y]][p->pos[dim_x] + 1].cost = p->cost + hiker_costs[m->map[p->pos[dim_y]][p->pos[dim_x] + 1] - 1];
        //m->hmap[p->pos[dim_y]][p->pos[dim_x] + 1] =p->cost + hiker_costs[m->map[p->pos[dim_y]][p->pos[dim_x] + 1]];
				path[p->pos[dim_y]][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y]][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] + 1].hn);
			//-------------------------------------------------------------------------------------
		}

		// Below - Left
		if(( path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost > (p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] - 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost = p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] - 1] - 1];
        //m->hmap[p->pos[dim_y] +1][p->pos[dim_x] - 1] =p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] - 1]];
				path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn);
			//-------------------------------------------------------------------------------------
		}

		// Below
		if(( path[p->pos[dim_y] + 1][p->pos[dim_x]].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost > (p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x]] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y] + 1][p->pos[dim_x]].cost = p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] ] - 1];
        //m->hmap[p->pos[dim_y] +1][p->pos[dim_x]] =p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x]]];
				path[p->pos[dim_y] + 1][p->pos[dim_x]].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] + 1][p->pos[dim_x]].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x]].hn);
			//-------------------------------------------------------------------------------------
		}

		// Below - Right
		if(( path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) &&(path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > (p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] + 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost = p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] + 1] - 1];
        //m->hmap[p->pos[dim_y] +1][p->pos[dim_x] + 1] =p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] + 1]];
				path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
			//-------------------------------------------------------------------------------------
		}
     
  }
  printf("\n");
  printf("PATH FINDING FOR HIKER\n");


  for (y = 0; y < MAP_Y; y++) {
	 	for (x = 0; x < MAP_X; x++) {
      
      if(mapxy(x, y) == ter_boulder){
        putchar('B');
        putchar('B');
        putchar(' ');
      }
      else if((x==0 || x == 79 || y==0 || y == 20) && mapxy(x, y) == ter_path){
        putchar('G');
        putchar('G');
        putchar(' ');
      }
      else if(mapxy(x, y) == ter_water){
        putchar('W');
        putchar('W');
        putchar(' ');
      }
      else if(mapxy(x, y) == ter_tree){
        putchar('T');
        putchar('T');
        putchar(' ');
      }
      // else if(mapxy(x, y) == ter_mountain){
      //   putchar('M');
      //   putchar('M');
      //   putchar(' ');
      // }
      // else if(mapxy(x, y) == ter_forest){
      //   putchar('F');
      //   putchar('F');
      //   putchar(' ');
      // }
      else if (mapxy(x, y) == player_pc){
        putchar('@');
        putchar('@');
        putchar(' ');
      }
      else {
        if (path[y][x].cost % 100 < 10){
          putchar(' ');
          printf("%d", path[y][x].cost % 100);
        }
        else{
          printf("%d", path[y][x].cost % 100);
        }
        
        putchar(' ');
      }
    }
     putchar('\n');
  }
 heap_delete(&h);
 printf("\n");

}



static void dijkstra_for_rival(map_t *m)
{
  static path_t path[MAP_Y][MAP_X], *p;

  uint32_t initialized = 0;

  heap_t h;

  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  

  for (y = 0; y < MAP_Y; y++){
		for (x = 0; x < MAP_X; x++){
			path[y][x].cost = INT_MAX;
		}
	} 



  path[m->pc[dim_x]][m->pc[dim_y]].cost = 0;

  //printf("%d\n", m->pc[dim_y]);
  heap_init(&h, path_cmp, NULL);

  for (y = 0; y < MAP_Y; y++) {

    for (x = 0; x < MAP_X; x++) {
       if(mapxy(x, y) != ter_boulder && mapxy(x, y) != ter_tree && mapxy(x, y) != ter_water && mapxy(x, y) != ter_mountain && mapxy(x, y) != ter_forest){

          path[y][x].hn = heap_insert(&h, &path[y][x]);

      }
      else{

        path[y][x].hn = NULL;

      }

    }

  }

  
  

  while ((p = heap_remove_min(&h))) {
      // m->hmap[p->pos[dim_y]][p->pos[dim_x]] = p->cost;
      p->hn = NULL;
      
      //printf("%d\n", p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1]]);
      //printf("%d %d", p->pos[dim_y], p->pos[dim_x]);
      

    //Top left
    if(( path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost > (p->cost + rival_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1] - 1]))){
        // ------------------------------------------------------------------------------------
        path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].cost = p->cost + rival_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1] - 1];
        //m->hmap[p->pos[dim_y] -1][p->pos[dim_x] - 1] =p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1]];
				path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] - 1].hn);
        // ------------------------------------------------------------------------------------
    }
    
    //printf("%d\n", hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1]]);
    //Top
    if(( path[p->pos[dim_y] - 1][p->pos[dim_x]].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x]].cost > (p->cost + rival_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] - 1] - 1]))){
		// ------------------------------------------------------------------------------------
			path[p->pos[dim_y] - 1][p->pos[dim_x]].cost = p->cost + rival_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x]] - 1];
      //m->hmap[p->pos[dim_y] -1][p->pos[dim_x]] =p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x]]];
			path[p->pos[dim_y] - 1][p->pos[dim_x]].from[dim_y] = p->pos[dim_y];
			path[p->pos[dim_y] - 1][p->pos[dim_x]].from[dim_x] = p->pos[dim_x];
			heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x]].hn);
		//-------------------------------------------------------------------------------------
		}

    //Top Right
    if(( path[p->pos[dim_y] -1][p->pos[dim_x] + 1].hn) && (path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost > (p->cost + rival_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] + 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].cost = p->cost + rival_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] + 1] - 1];
        //m->hmap[p->pos[dim_y] -1][p->pos[dim_x] + 1] =p->cost + hiker_costs[m->map[p->pos[dim_y] - 1][p->pos[dim_x] + 1]];
				path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1][p->pos[dim_x] + 1].hn);
			//-------------------------------------------------------------------------------------
		}


    // Left
		if(( path[p->pos[dim_y]][p->pos[dim_x] - 1].hn) &&(path[p->pos[dim_y]][p->pos[dim_x] - 1].cost > (p->cost + rival_costs[m->map[p->pos[dim_y]][p->pos[dim_x] - 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y]][p->pos[dim_x] - 1].cost = p->cost + rival_costs[m->map[p->pos[dim_y]][p->pos[dim_x] - 1] - 1];
        //m->hmap[p->pos[dim_y]][p->pos[dim_x] - 1] =p->cost + hiker_costs[m->map[p->pos[dim_y]][p->pos[dim_x] - 1]];
				path[p->pos[dim_y]][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y]][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] - 1].hn);
			//-------------------------------------------------------------------------------------
		}

    // Right
		if(( path[p->pos[dim_y]][p->pos[dim_x] + 1].hn) &&(path[p->pos[dim_y]][p->pos[dim_x] + 1].cost > (p->cost + rival_costs[m->map[p->pos[dim_y]][p->pos[dim_x] + 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y]][p->pos[dim_x] + 1].cost = p->cost + rival_costs[m->map[p->pos[dim_y]][p->pos[dim_x] + 1] - 1];
        //m->hmap[p->pos[dim_y]][p->pos[dim_x] + 1] =p->cost + hiker_costs[m->map[p->pos[dim_y]][p->pos[dim_x] + 1]];
				path[p->pos[dim_y]][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y]][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y]][p->pos[dim_x] + 1].hn);
			//-------------------------------------------------------------------------------------
		}

		// Below - Left
		if(( path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost > (p->cost + rival_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] - 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].cost = p->cost + rival_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] - 1] - 1];
        //m->hmap[p->pos[dim_y] +1][p->pos[dim_x] - 1] =p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] - 1]];
				path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] - 1].hn);
			//-------------------------------------------------------------------------------------
		}

		// Below
		if(( path[p->pos[dim_y] + 1][p->pos[dim_x]].hn) && (path[p->pos[dim_y] + 1][p->pos[dim_x]].cost > (p->cost + rival_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x]] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y] + 1][p->pos[dim_x]].cost = p->cost + rival_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] ] - 1];
        //m->hmap[p->pos[dim_y] +1][p->pos[dim_x]] =p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x]]];
				path[p->pos[dim_y] + 1][p->pos[dim_x]].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] + 1][p->pos[dim_x]].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x]].hn);
			//-------------------------------------------------------------------------------------
		}

		// Below - Right
		if(( path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn) &&(path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost > (p->cost + rival_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] + 1] - 1]))){
			// ------------------------------------------------------------------------------------
				path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].cost = p->cost + rival_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] + 1] - 1];
        //m->hmap[p->pos[dim_y] +1][p->pos[dim_x] + 1] =p->cost + hiker_costs[m->map[p->pos[dim_y] + 1][p->pos[dim_x] + 1]];
				path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
				path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
				heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1][p->pos[dim_x] + 1].hn);
			//-------------------------------------------------------------------------------------
		}
     
  }
  printf("\n");
  printf("PATH FINDING FOR RIVAL\n");


  for (y = 0; y < MAP_Y; y++) {
	 	for (x = 0; x < MAP_X; x++) {
      
      if(mapxy(x, y) == ter_boulder){
        putchar('B');
        putchar('B');
        putchar(' ');
      }
      else if((x==0 || x == 79 || y==0 || y == 20) && mapxy(x, y) == ter_path){
        putchar('G');
        putchar('G');
        putchar(' ');
      }
      else if(mapxy(x, y) == ter_water){
        putchar('W');
        putchar('W');
        putchar(' ');
      }
      else if(mapxy(x, y) == ter_tree){
        putchar('T');
        putchar('T');
        putchar(' ');
      }
      else if(mapxy(x, y) == ter_mountain){
        putchar('M');
        putchar('M');
        putchar(' ');
      }
      else if(mapxy(x, y) == ter_forest){
        putchar('F');
        putchar('F');
        putchar(' ');
      }
      else if (mapxy(x, y) == player_pc){
        putchar('@');
        putchar('@');
        putchar(' ');
      }
      else {
        if (path[y][x].cost % 100 < 10){
          putchar(' ');
          printf("%d", path[y][x].cost % 100);
        }
        else{
          printf("%d", path[y][x].cost % 100);
        }
        
        putchar(' ');
      }
    }
     putchar('\n');
  }
 heap_delete(&h);
 printf("\n");

}


// void print_hiker(map_t *m)
// {

// 	printf("\n Hiker Dijkstras Cost Map\n");
// 	int y, x;



//   // for (y = 0; y < MAP_Y; y++) {
// 	// 	for (x = 0; x < MAP_X; x++) {
//   //     printf("%d\n", m->hmap[y][x]);
//   //   }
//   // }



// 	for (y = 0; y < MAP_Y; y++) {
// 		for (x = 0; x < MAP_X; x++) {
// 			switch (mapxy(x,y)) {
// 				case ter_boulder:
// 					putchar('X');
//           putchar('X');
//           //putchar(' ');
// 					break;
//         case ter_tree:
// 					putchar('X');
//           putchar('X');
//           //putchar(' ');
// 					break;
//         case ter_water:
// 					putchar('X');
//           putchar('X');
//          // putchar(' ');
// 					break;

//         // case player_pc:
//         //   putchar('0');
//         //     putchar(' ');
//         //     //putchar(' ');
// 				// 		break;

// 				default:
// 					if (y == m->pc[dim_y] && x == m->pc[dim_x]){
// 						putchar('@');
//             putchar('@');
//             //putchar(' ');
// 						break;
// 					}
// 					if(m->hmap[y][x] != 0){
//             if (m->hmap[y][x] % 100 < 10){
//               putchar('0');
//               printf("%d", m->hmap[y][x] % 100);
//             }else{
// 						  printf("%d", m->hmap[y][x] % 100);
//               //putchar(' ');
// 						  break;
//             }
// 					}else{
// 						putchar(' ');
//             putchar(' ');

// 					}
// 			}
//     	}
//     	putchar('\n');
// 	}
// }

static int build_paths(map_t *m)
{
  pair_t from, to;

  from[dim_x] = 1;
  to[dim_x] = MAP_X - 2;
  from[dim_y] = m->w;
  to[dim_y] = m->e;

  dijkstra_path(m, from, to);

  from[dim_y] = 1;
  to[dim_y] = MAP_Y - 2;
  from[dim_x] = m->n;
  to[dim_x] = m->s;

  dijkstra_path(m, from, to);

  return 0;
}

static int gaussian[5][5] = {
  {  1,  4,  7,  4,  1 },
  {  4, 16, 26, 16,  4 },
  {  7, 26, 41, 26,  7 },
  {  4, 16, 26, 16,  4 },
  {  1,  4,  7,  4,  1 }
};
static int smooth_height(map_t *m)
{
  map_t mr;
  if (m == NULL){
     m = &mr;
  }


  //map_t mr;
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
  /*  FILE *out;*/
  uint8_t height[MAP_Y][MAP_X];

  memset(&height, 0, sizeof (height));

  /* Seed with some values */
  for (i = 1; i < 255; i += 20) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (height[y][x]);
    height[y][x] = i;
    if (i == 1) {
      head = tail = malloc(sizeof (*tail));
    } else {
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);
  */
  
  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = height[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !height[y - 1][x - 1]) {
      height[y - 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !height[y][x - 1]) {
      height[y][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < MAP_Y && !height[y + 1][x - 1]) {
      height[y + 1][x - 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !height[y - 1][x]) {
      height[y - 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < MAP_Y && !height[y + 1][x]) {
      height[y + 1][x] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < MAP_X && y - 1 >= 0 && !height[y - 1][x + 1]) {
      height[y - 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < MAP_X && !height[y][x + 1]) {
      height[y][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < MAP_X && y + 1 < MAP_Y && !height[y + 1][x + 1]) {
      height[y + 1][x + 1] = i;
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y + 1;
    }

    tmp = head;
    head = head->next;
    free(tmp);
  }
  
  /* And smooth it a bit with a gaussian convolution */
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      
      mr.height[y][x] = t / s;
    }
  }
  /* Let's do it again, until it's smooth like Kenny G. */
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      mr.height[y][x] = t / s;
    }
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);

  out = fopen("smoothed.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->height, sizeof (m->height), 1, out);
  fclose(out);
  */
  m = &mr;
  return 0;
}

static void find_building_location(map_t *m, pair_t p)
{
  do {
    p[dim_x] = rand() % (MAP_X - 3) + 1;
    p[dim_y] = rand() % (MAP_Y - 3) + 1;

    if ((((mapxy(p[dim_x] - 1, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] - 1, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x] + 2, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] + 2, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] - 1) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] - 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] + 2) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 2) == ter_path)))   &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_center))) &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_path)))) {
          break;
    }
  } while (1);
}

static int place_pokemart(map_t *m, int x_loco, int y_loco)
{
  pair_t p;

  find_building_location(m, p);

  double dis = sqrt(pow(x_loco - 200, 2) + pow(y_loco - 200, 2));
  
  double chance = ((-45 * dis) / 200 + 50);
  double rr = rand() % 99 + 1;
  // printf("%f %f\n", chance, rr);
  if (chance <= rr && x_loco != 200 && y_loco != 200){
    return 0;
  }

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_mart;

  return 0;
}

static int place_center(map_t *m, int x_loco, int y_loco)
{  pair_t p;

  find_building_location(m, p);

  double dis = sqrt(pow(x_loco - 200, 2) + pow(y_loco - 200, 2));
  
  double chance = ((-45 * dis) / 200 + 50);
  double rr = rand() % 99 + 1;
  //printf("%f %f\n", chance, rr);
  if (chance <= rr){
    return 0;
  }
  else{mapxy(p[dim_x]    , p[dim_y]    ) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_center;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_center;}

  

  return 0;
}

/* Chooses tree or boulder for border cell.  Choice is biased by dominance *
 * of neighboring cells.                                                   */
static terrain_type_t border_type(map_t *m, int32_t x, int32_t y)
{
  int32_t p, q;
  int32_t r, t;
  int32_t miny, minx, maxy, maxx;
  
  r = t = 0;
  
  miny = y - 1 >= 0 ? y - 1 : 0;
  maxy = y + 1 <= MAP_Y ? y + 1: MAP_Y;
  minx = x - 1 >= 0 ? x - 1 : 0;
  maxx = x + 1 <= MAP_X ? x + 1: MAP_X;

  for (q = miny; q < maxy; q++) {
    for (p = minx; p < maxx; p++) {
      if (q != y || p != x) {
        if (m->map[q][p] == ter_mountain ||
            m->map[q][p] == ter_boulder) {
          r++;
        } else if (m->map[q][p] == ter_forest ||
                   m->map[q][p] == ter_tree) {
          t++;
        }
      }
    }
  }
  
  if (t == r) {
    return rand() & 1 ? ter_boulder : ter_tree;
  } else if (t > r) {
    if (rand() % 10) {
      return ter_tree;
    } else {
      return ter_boulder;
    }
  } else {
    if (rand() % 10) {
      return ter_boulder;
    } else {
      return ter_tree;
    }
  }
}

static void place_pc(map_t *m){
  int notPlaced = 0;
  int x_pc;
  int y_pc;
  while(!notPlaced){
    x_pc = rand() % 78 + 1;
    y_pc = rand() % 18 + 1;
    if (m->map[x_pc][y_pc] == ter_path){
      m->map[x_pc][y_pc] = player_pc;
      m->pc[dim_x] = x_pc;
      m->pc[dim_y] = y_pc;
      notPlaced = 1;
    }
  }
}

static int map_terrain(map_t* m[401][401], int x_loco, int y_loco)
{
  uint8_t n, s, e, w; 
  
  map_t mr;
  if (m[x_loco][y_loco] == NULL){
    m[x_loco][y_loco] = &mr;
  }
 


  if (m[x_loco][y_loco+1] == NULL){
    s = 1 + rand() % (MAP_X - 2);
  }else{
    s = m[x_loco][y_loco+1]->n;
    
  }

  if (m[x_loco][y_loco-1] == NULL){
    n = 1 + rand() % (MAP_X - 2);
    
  }else{
    n = m[x_loco][y_loco-1]->s;
    
  }

  if (m[x_loco+1][y_loco] == NULL){
    e = 1 + rand() % (MAP_Y - 2);
  }else{
    e = m[x_loco+1][y_loco]->w;
  }

  if (m[x_loco-1][y_loco] == NULL){
    w = 1 + rand() % (MAP_Y - 2);
  }else{
    w = m[x_loco-1][y_loco]->e;
  }

  int32_t i, x, y;
  queue_node_t *head, *tail, *tmp;
  //  FILE *out;
  int num_grass, num_clearing, num_mountain, num_forest, num_water, num_total;
  terrain_type_t type;
  int added_current = 0;
  
  num_grass = rand() % 4 + 2;
  num_clearing = rand() % 4 + 2;
  num_mountain = rand() % 2 + 1;
  num_forest = rand() % 2 + 1;
  num_water = rand() % 2 + 1;
  num_total = num_grass + num_clearing + num_mountain + num_forest + num_water;

  memset(&m[x_loco][y_loco]->map, 0, sizeof (m[x_loco][y_loco]->map));

  /* Seed with some values */
  for (i = 0; i < num_total; i++) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (m[x_loco][y_loco]->map[y][x]);
    if (i == 0) {
      type = ter_grass;
    } else if (i == num_grass) {
      type = ter_clearing;
    } else if (i == num_grass + num_clearing) {
      type = ter_mountain;
    } else if (i == num_grass + num_clearing + num_mountain) {
      type = ter_forest;
    } else if (i == num_grass + num_clearing + num_mountain + num_forest) {
      type = ter_water;
    }
    m[x_loco][y_loco]->map[y][x] = type;
    if (i == 0) {
      head = tail = malloc(sizeof (*tail));
    } else {
      tail->next = malloc(sizeof (*tail));
      tail = tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */
  
  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = m[x_loco][y_loco]->map[y][x];
    
    if (x - 1 >= 0 && !m[x_loco][y_loco]->map[y][x - 1]) {
      if ((rand() % 100) < 80) {
        m[x_loco][y_loco]->map[y][x - 1] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x - 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m[x_loco][y_loco]->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y - 1 >= 0 && !m[x_loco][y_loco]->map[y - 1][x]) {
      if ((rand() % 100) < 20) {
        m[x_loco][y_loco]->map[y - 1][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y - 1;
      } else if (!added_current) {
        added_current = 1;
        m[x_loco][y_loco]->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y + 1 < MAP_Y && !m[x_loco][y_loco]->map[y + 1][x]) {
      if ((rand() % 100) < 20) {
        m[x_loco][y_loco]->map[y + 1][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y + 1;
      } else if (!added_current) {
        added_current = 1;
        m[x_loco][y_loco]->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (x + 1 < MAP_X && !m[x_loco][y_loco]->map[y][x + 1]) {
      if ((rand() % 100) < 80) {
        m[x_loco][y_loco]->map[y][x + 1] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x + 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m[x_loco][y_loco]->map[y][x] = i;
        tail->next = malloc(sizeof (*tail));
        tail = tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    added_current = 0;
    tmp = head;
    head = head->next;
    free(tmp);
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (y == 0 || y == MAP_Y - 1 ||
          x == 0 || x == MAP_X - 1) {
          mapxyz(x, y) = type = border_type(m[x_loco][y_loco], x, y);
      }
    }
  }

  m[x_loco][y_loco]->n = n;
  m[x_loco][y_loco]->s = s;
  m[x_loco][y_loco]->e = e;
  m[x_loco][y_loco]->w = w;
  //printf("%d\n", m[x_loco][y_loco]->s);
  
  mapxyz(n,         0        ) = ter_path;
  mapxyz(n,         1        ) = ter_path;
  mapxyz(s,         MAP_Y - 1) = ter_path;
  mapxyz(s,         MAP_Y - 2) = ter_path;
  mapxyz(0,         w        ) = ter_path;
  mapxyz(1,         w        ) = ter_path;
  mapxyz(MAP_X - 1, e        ) = ter_path;
  mapxyz(MAP_X - 2, e        ) = ter_path;
  //m[x_loco][y_loco] = &mr;
  return 0;
}

static int place_boulders(map_t *m)
{
  int i;
  int x, y;

  for (i = 0; i < MIN_BOULDERS || rand() % 100 < BOULDER_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_forest && m->map[y][x] != ter_path) {
      m->map[y][x] = ter_boulder;
    }
  }

  return 0;
}

static int place_trees(map_t *m)
{
  int i;
  int x, y;
  
  for (i = 0; i < MIN_TREES || rand() % 100 < TREE_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_mountain &&
        m->map[y][x] != ter_path     &&
        m->map[y][x] != ter_water) {
        m->map[y][x] = ter_tree;
    }
  }

  return 0;
}

static int new_map(map_t* m[401][401], int x_loco, int y_loco)
{
  smooth_height(m[x_loco][y_loco]);
  
  map_terrain(m, x_loco, y_loco);
  place_boulders(m[x_loco][y_loco]);
  place_trees(m[x_loco][y_loco]);
  build_paths(m[x_loco][y_loco]);
  place_pokemart(m[x_loco][y_loco], x_loco, y_loco);
  place_center(m[x_loco][y_loco], x_loco, y_loco);
  place_pc(m[x_loco][y_loco]);
  dijkstra_for_hiker(m[x_loco][y_loco]);
  dijkstra_for_rival(m[x_loco][x_loco]);
  //pair_t from, to;

  // from[dim_x] = 1;
  // to[dim_x] = pc_x;
  // from[dim_y] = 1;
  // to[dim_y] = pc_y;
  // dijkstra_for_players(m[x_loco][y_loco], from, to);

  return 0;
}

static void print_map(map_t *m[401][401], int x_loco, int y_loco)
{
  int x, y;
  int default_reached = 0;
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      switch (m[x_loco][y_loco]->map[y][x]) {
      case ter_boulder:
      case ter_mountain:
        putchar('%');
        break;
      case ter_tree:
      case ter_forest:
        putchar('^');
        break;
      case ter_path:
        //stop road placement on left side of the map
        if(x_loco == 0 && x == 0){
          putchar('%');
          break;
        }
        

        if(x_loco == 398 && x == 79){
          putchar('%');
          break;
        }

        if(y_loco == 0 && y == 0){
          putchar('%');
          break;
        }

        if(y_loco == 398 && y == 20){
          putchar('%');
          break;
        }
        putchar('#');
        break;
      case ter_mart:
        putchar('M');
        break;
      case ter_center:
        putchar('C');
        break;
      case ter_grass:
        putchar(':');
        break;
      case ter_clearing:
        putchar('.');
        break;
      case ter_water:
        putchar('~');
        break;
      case player_pc:
        putchar('@');
        break;
      default:
        default_reached = 1;
        break;
      }
    }
    putchar('\n');
  }

  if (default_reached) {
    fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
  }
}


int main(int argc, char *argv[])
{
  map_t *ent[401][401];

  int tempx;
  int tempy;
  
  int x_loco = 200;
  int y_loco = 200;
  struct timeval tv;
  uint32_t seed;
  int in_game = 1;
  char c;

  if (argc == 2) {
    seed = atoi(argv[1]);
  } else {
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }
  // printf("Using seed: %u\n", seed);
  srand(seed);

  for(int i = 0; i < 400; ++i){
        for(int j = 0; j < 400; ++j){
             ent[i][j] = NULL;
      }
  }
  ent[x_loco][y_loco] = malloc(sizeof (*ent[x_loco][y_loco]));
  new_map(ent, x_loco, y_loco);
  print_map(ent, x_loco, y_loco);
  //print_hiker(ent[x_loco][y_loco]);
  
  //printf("%d\n", ent[x_loco][y_loco]->s);
   while(in_game){
        printf("Location: %d, %d     ", x_loco - 200, y_loco - 200);
        printf("Enter a command: ");
        scanf(" %c", &c);
        switch(c){
        case 'n':
            if (y_loco == 0){
              printf("Error: Enter Valid Location\n");
              break;
            }
            y_loco -= 1;
            if (ent[x_loco][y_loco] == NULL){
              ent[x_loco][y_loco] = malloc(sizeof (*ent[x_loco][y_loco]));
              new_map(ent, x_loco, y_loco);
            }
            
            print_map(ent, x_loco, y_loco);
            
            break;
        
        case 's':
            if (y_loco == 398){
                printf("Error: Enter Valid Location\n");
                break;
            }
            y_loco += 1;
            if (ent[x_loco][y_loco] == NULL){
              ent[x_loco][y_loco] = malloc(sizeof (*ent[x_loco][y_loco]));
              new_map(ent, x_loco, y_loco);
            }
            print_map(ent, x_loco, y_loco);
            break;
        
        case 'e':            
            if (x_loco == 398){
                printf("Error: Enter Valid Location\n");
                break;
            }
            x_loco += 1;
            if (ent[x_loco][y_loco] == NULL){
              ent[x_loco][y_loco] = malloc(sizeof (*ent[x_loco][y_loco]));
              new_map(ent, x_loco, y_loco);
            }
            print_map(ent, x_loco, y_loco);
            break;

        case 'w':
            if (x_loco == 0){
                printf("Error: Enter Valid Location\n");
                break;
            }
            x_loco -= 1;
            if (ent[x_loco][y_loco] == NULL){
              ent[x_loco][y_loco] = malloc(sizeof (*ent[x_loco][y_loco]));
              new_map(ent, x_loco, y_loco);
            }
            print_map(ent, x_loco, y_loco);
            break;

        case 'q':
            in_game = 0;
            break;
            
        
        case 'f':
            tempx = x_loco;
            tempy = y_loco;
            scanf(" %d %d", &x_loco, &y_loco);
            
            //printf("%d %d\n", x_loco, y_loco);
            x_loco += 200;
            y_loco += 200;

            if (x_loco > 398 || x_loco < 0 || y_loco > 398 || y_loco < 0){
              printf("Error: Enter Valid Locations \n");
              x_loco = tempx;
              y_loco = tempy;
              break;
            }
            //printf("%d %d\n", x_loco, y_loco);
            if (ent[x_loco][y_loco] == NULL){
              ent[x_loco][y_loco] = malloc(sizeof (*ent[x_loco][y_loco]));
              new_map(ent, x_loco, y_loco);
            }
            print_map(ent, x_loco, y_loco);
            break;

        default:
            printf("Error: Input Valid Command");
            break;
    }

  
  }
  for(int i = 0; i < 400; ++i){
        for(int j = 0; j < 400; ++j){
          if (ent[i][j] != NULL){
              free(ent[i][j]);
          }
      }
  }
return 0; 
}
