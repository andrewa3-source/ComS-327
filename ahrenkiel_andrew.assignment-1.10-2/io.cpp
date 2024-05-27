#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

#include "io.h"
#include "character.h"
#include "poke327.h"
#include "pokemon.h"

typedef struct io_message {
  /* Will print " --more-- " at end of line when another message follows. *
   * Leave 10 extra spaces for that.                                      */
  char msg[71];
  struct io_message *next;
} io_message_t;

static io_message_t *io_head, *io_tail;

void io_init_terminal(void)
{
  initscr();
  raw();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();
  init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
  init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
}

void io_reset_terminal(void)
{
  endwin();

  while (io_head) {
    io_tail = io_head;
    io_head = io_head->next;
    free(io_tail);
  }
  io_tail = NULL;
}

void io_queue_message(const char *format, ...)
{
  io_message_t *tmp;
  va_list ap;

  if (!(tmp = (io_message_t *) malloc(sizeof (*tmp)))) {
    perror("malloc");
    exit(1);
  }

  tmp->next = NULL;

  va_start(ap, format);

  vsnprintf(tmp->msg, sizeof (tmp->msg), format, ap);

  va_end(ap);

  if (!io_head) {
    io_head = io_tail = tmp;
  } else {
    io_tail->next = tmp;
    io_tail = tmp;
  }
}

static void io_print_message_queue(uint32_t y, uint32_t x)
{
  while (io_head) {
    io_tail = io_head;
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(y, x, "%-80s", io_head->msg);
    attroff(COLOR_PAIR(COLOR_CYAN));
    io_head = io_head->next;
    if (io_head) {
      attron(COLOR_PAIR(COLOR_CYAN));
      mvprintw(y, x + 70, "%10s", " --more-- ");
      attroff(COLOR_PAIR(COLOR_CYAN));
      refresh();
      getch();
    }
    free(io_tail);
  }
  io_tail = NULL;
}

/**************************************************************************
 * Compares trainer distances from the PC according to the rival distance *
 * map.  This gives the approximate distance that the PC must travel to   *
 * get to the trainer (doesn't account for crossing buildings).  This is  *
 * not the distance from the NPC to the PC unless the NPC is a rival.     *
 *                                                                        *
 * Not a bug.                                                             *
 **************************************************************************/
static int compare_trainer_distance(const void *v1, const void *v2)
{
  const character *const *c1 = (const character * const *) v1;
  const character *const *c2 = (const character * const *) v2;

  return (world.rival_dist[(*c1)->pos[dim_y]][(*c1)->pos[dim_x]] -
          world.rival_dist[(*c2)->pos[dim_y]][(*c2)->pos[dim_x]]);
}

static character *io_nearest_visible_trainer()
{
  character **c, *n;
  uint32_t x, y, count;

  c = (character **) malloc(world.cur_map->num_trainers * sizeof (*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
          &world.pc) {
        c[count++] = world.cur_map->cmap[y][x];
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof (*c), compare_trainer_distance);

  n = c[0];

  free(c);

  return n;
}

void io_display()
{
  uint32_t y, x;
  character *c;

  clear();
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.cur_map->cmap[y][x]) {
        mvaddch(y + 1, x, world.cur_map->cmap[y][x]->symbol);
      } else {
        switch (world.cur_map->map[y][x]) {
        case ter_boulder:
          attron(COLOR_PAIR(COLOR_MAGENTA));
          mvaddch(y + 1, x, BOULDER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_MAGENTA));
          break;
        case ter_mountain:
          attron(COLOR_PAIR(COLOR_MAGENTA));
          mvaddch(y + 1, x, MOUNTAIN_SYMBOL);
          attroff(COLOR_PAIR(COLOR_MAGENTA));
          break;
        case ter_tree:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, TREE_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_forest:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, FOREST_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_path:
          attron(COLOR_PAIR(COLOR_YELLOW));
          mvaddch(y + 1, x, PATH_SYMBOL);
          attroff(COLOR_PAIR(COLOR_YELLOW));
          break;
        case ter_gate:
          attron(COLOR_PAIR(COLOR_YELLOW));
          mvaddch(y + 1, x, GATE_SYMBOL);
          attroff(COLOR_PAIR(COLOR_YELLOW));
          break;
        case ter_mart:
          attron(COLOR_PAIR(COLOR_BLUE));
          mvaddch(y + 1, x, POKEMART_SYMBOL);
          attroff(COLOR_PAIR(COLOR_BLUE));
          break;
        case ter_center:
          attron(COLOR_PAIR(COLOR_RED));
          mvaddch(y + 1, x, POKEMON_CENTER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_RED));
          break;
        case ter_gym:
          attron(COLOR_PAIR(COLOR_RED));
          mvaddch(y + 1, x, GYM_SYMBOL);
          attroff(COLOR_PAIR(COLOR_RED));
          break;
        case ter_grass:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, TALL_GRASS_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_clearing:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, SHORT_GRASS_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_water:
          attron(COLOR_PAIR(COLOR_CYAN));
          mvaddch(y + 1, x, WATER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_CYAN));
          break;
        
        default:
          attron(COLOR_PAIR(COLOR_CYAN));
          mvaddch(y + 1, x, ERROR_SYMBOL);
          attroff(COLOR_PAIR(COLOR_CYAN)); 
       }
      }
    }
  }

  mvprintw(23, 1, "PC position is (%2d,%2d) on map %d%cx%d%c.",
           world.pc.pos[dim_x],
           world.pc.pos[dim_y],
           abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_x] - (WORLD_SIZE / 2) >= 0 ? 'E' : 'W',
           abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_y] - (WORLD_SIZE / 2) <= 0 ? 'N' : 'S');
  mvprintw(22, 1, "%d known %s.", world.cur_map->num_trainers,
           world.cur_map->num_trainers > 1 ? "trainers" : "trainer");
  mvprintw(22, 30, "Nearest visible trainer: ");
  if ((c = io_nearest_visible_trainer())) {
    attron(COLOR_PAIR(COLOR_RED));
    mvprintw(22, 55, "%c at vector %d%cx%d%c.",
             c->symbol,
             abs(c->pos[dim_y] - world.pc.pos[dim_y]),
             ((c->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ?
              'N' : 'S'),
             abs(c->pos[dim_x] - world.pc.pos[dim_x]),
             ((c->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ?
              'W' : 'E'));
    attroff(COLOR_PAIR(COLOR_RED));
  } else {
    attron(COLOR_PAIR(COLOR_BLUE));
    mvprintw(22, 55, "NONE.");
    attroff(COLOR_PAIR(COLOR_BLUE));
  }

  io_print_message_queue(0, 0);

  refresh();
}

uint32_t io_teleport_pc(pair_t dest)
{
  /* Just for fun. And debugging.  Mostly debugging. */

  do {
    dest[dim_x] = rand_range(1, MAP_X - 2);
    dest[dim_y] = rand_range(1, MAP_Y - 2);
  } while (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]                  ||
           move_cost[char_pc][world.cur_map->map[dest[dim_y]]
                                                [dest[dim_x]]] == INT_MAX ||
           world.rival_dist[dest[dim_y]][dest[dim_x]] < 0);

  return 0;
}

static void io_scroll_trainer_list(char (*s)[40], uint32_t count)
{
  uint32_t offset;
  uint32_t i;

  offset = 0;

  while (1) {
    for (i = 0; i < 13; i++) {
      mvprintw(i + 6, 19, " %-40s ", s[i + offset]);
    }
    switch (getch()) {
    case KEY_UP:
      if (offset) {
        offset--;
      }
      break;
    case KEY_DOWN:
      if (offset < (count - 13)) {
        offset++;
      }
      break;
    case 27:
      return;
    }

  }
}

static void io_list_trainers_display(npc **c,
                                     uint32_t count)
{
  uint32_t i;
  char (*s)[40]; /* pointer to array of 40 char */

  s = (char (*)[40]) malloc(count * sizeof (*s));

  mvprintw(3, 19, " %-40s ", "");
  /* Borrow the first element of our array for this string: */
  snprintf(s[0], 40, "You know of %d trainers:", count);
  mvprintw(4, 19, " %-40s ", *s);
  mvprintw(5, 19, " %-40s ", "");

  for (i = 0; i < count; i++) {
    snprintf(s[i], 40, "%16s %c: %2d %s by %2d %s",
             char_type_name[c[i]->ctype],
             c[i]->symbol,
             abs(c[i]->pos[dim_y] - world.pc.pos[dim_y]),
             ((c[i]->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ?
              "North" : "South"),
             abs(c[i]->pos[dim_x] - world.pc.pos[dim_x]),
             ((c[i]->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ?
              "West" : "East"));
    if (count <= 13) {
      /* Handle the non-scrolling case right here. *
       * Scrolling in another function.            */
      mvprintw(i + 6, 19, " %-40s ", s[i]);
    }
  }

  if (count <= 13) {
    mvprintw(count + 6, 19, " %-40s ", "");
    mvprintw(count + 7, 19, " %-40s ", "Hit escape to continue.");
    while (getch() != 27 /* escape */)
      ;
  } else {
    mvprintw(19, 19, " %-40s ", "");
    mvprintw(20, 19, " %-40s ",
             "Arrows to scroll, escape to continue.");
    io_scroll_trainer_list(s, count);
  }

  free(s);
}

static void io_list_trainers()
{
  npc **c;
  uint32_t x, y, count;

  c = (npc **) malloc(world.cur_map->num_trainers * sizeof (*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
          &world.pc) {
        c[count++] = dynamic_cast<npc *> (world.cur_map->cmap[y][x]);
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof (*c), compare_trainer_distance);

  /* Display it */
  io_list_trainers_display(c, count);
  free(c);

  /* And redraw the map */
  io_display();
}

void io_pokemart()
{
  mvprintw(0, 0, "Welcome to the Pokemart.  Could I interest you in some Pokeballs?");
  refresh();
  getch();
}

void io_pokemon_center()
{
  mvprintw(0, 0, "Welcome to the Pokemon Center.  How can Nurse Joy assist you?");
  getch();
  mvprintw(0, 0, "                                                             ");
  mvprintw(0, 0, "press 1 to heal pokemon");
  int i = getch();
  if (i == '1'){
    for (int j = 0; j < 6; j++){
      if (world.pc.buddy[j] != NULL){
        world.pc.buddy[j]->hp = world.pc.buddy[j]->get_hp();
      }
    }
  }
  mvprintw(0, 0, "                                                             ");
  mvprintw(0, 0, "Your pokemon are good to go! Thanks for stopping in!");
  refresh();
  getch();
}




void init_battle_screen(){
  mvprintw(3, 19, " %-40s ", "");
  mvprintw(4, 19, " %-40s ", "");
  mvprintw(5, 19, " %-40s ", "");
  mvprintw(6, 19, " %-40s ", "");
  mvprintw(7, 19, " %-40s ", "");
  mvprintw(8, 19, " %-40s ", "");
  mvprintw(9, 19, " %-40s ", "");
  mvprintw(10, 19, " %-40s ", "");
  mvprintw(11, 19, " %-40s ", "");
  mvprintw(12, 19, " %-40s ", "");
  mvprintw(13, 19, " %-40s ", "");
  mvprintw(14, 19, " %-40s ", "");
  mvprintw(15, 19, " %-40s ", "");
  mvprintw(16, 19, " %-40s ", "");
  mvprintw(17, 19, " %-40s ", "");
  mvprintw(18, 19, " %-40s ", "");
  mvprintw(19, 19, " %-40s ", "");
}

pokemon choose_new_pokemon(character *current){
  int i = 0;
  int j = 0;
  while (true){
    if(current->symbol == '@'){
    init_battle_screen();
    mvprintw(9, 20, "Select a new Avaliable Pokemon");
    for(i = 0; i < 6; ++i){
      if(current->buddy[i] != NULL && current->buddy[i]->hp > 0){
        mvprintw(10 + i, 20, "%d: %s", i+1,  current->buddy[i]->get_species());
      }
      
    }
    i = getch();
    if (i == '1' && current->buddy[0] != NULL && current->buddy[0]->hp != 0){
      return *current->buddy[0];
    }
    else if (i == '2' && current->buddy[1] != NULL && current->buddy[1]->hp != 0){
      return *current->buddy[1];
    }
    else if (i == '3' && current->buddy[2] != NULL && current->buddy[2]->hp != 0){
      return *current->buddy[2];
    }
    else if (i == '4' && current->buddy[3] != NULL && current->buddy[3]->hp != 0){
      return *current->buddy[3];
    }
    else if (i == '5' && current->buddy[4] != NULL && current->buddy[4]->hp != 0){
      return *current->buddy[4];
    }
    else if (i == '6' && current->buddy[5] != NULL && current->buddy[5]->hp != 0){
      return *current->buddy[5];
    }
    else {
      mvprintw(8, 20, "Not a option, please select another");
      usleep(100000);
    }
  }
  else{
    for(i = 0; i < 6; ++i){
      if(current->buddy[i] != NULL && current->buddy[i]->hp > 0){
        j += 1;
      }
    }
    return *current->buddy[rand() % j];
  }
  }
}

void io_battle(character *aggressor, character *defender)
{
  npc *n = (npc *) ((aggressor == &world.pc) ? defender : aggressor);
  int i, j;
  bool in_battle = true;
  echo();
  curs_set(1);
  char (*s)[40]; /* pointer to array of 40 char */
  character *temp;
  if(aggressor->symbol == '@'){
    temp = defender;
    defender = aggressor;
    aggressor = temp;
  }

  s = (char (*)[40]) malloc(10 * sizeof (*s));

  init_battle_screen();

  for (i = 0; i < 6; ++i){
    if (aggressor->buddy[i] != NULL && aggressor->buddy[i] == 0){

    }
  }

  pokemon *current_def = defender->buddy[0];
  pokemon *current_agg = aggressor->buddy[0];
  int current_def_mhp = current_def->get_hp();
  int current_agg_mhp = current_agg->get_hp();
  



  do {
    init_battle_screen();

  int prx = 0;
  int pry = 0;
  for (i = 0; i < 6; ++i){
    if (aggressor->buddy[i] != NULL){
      mvprintw( 5 + pry , 58 + prx, "A");
      prx += 1;
      if (prx == 2){
        prx = 0;
        pry += 1;
      }
    }
    else {
      mvprintw( 5 + pry, 58 + prx, "*");
      prx += 1;
      if (prx == 2){
        prx = 0;
        pry += 1;
      }
    }
  }

  prx = 0;
  pry = 0;
  for (i = 0; i < 6; ++i){
    if (defender->buddy[i] != NULL){
      mvprintw( 13 + pry , 58 + prx, "A");
      prx += 1;
      if (prx == 2){
        prx = 0;
        pry += 1;
      }
    }
    else {
      mvprintw( 13 + pry, 58 + prx, "*");
      prx += 1;
      if (prx == 2){
        prx = 0;
        pry += 1;
      }
    }
  }

  mvprintw(3, 45, "%s", current_agg->get_species());
  mvprintw(5, 48, ">*)");
  mvprintw(6, 48, "( \\");
  mvprintw(7, 49, "^");
  mvprintw(6, 40, "%d/%d", current_agg->hp, current_agg_mhp);

  mvprintw(11, 27, "(*<");
  mvprintw(12, 27, "/ )");
  mvprintw(13, 28, "L");
  mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
  mvprintw(14, 25, "%s", current_def->get_species());

    mvprintw( 3, 19, "Battle !!");
    
    
    mvprintw(16, 20, "1. Attack  2. Item  3. Run  4. Pokemon  ");
    mvprintw(17, 19, "                                         ");
    mvprintw(18, 19, "                                         ");
    mvprintw(19, 19, "                                         ");

    refresh();
    move(1,1);
    i = getch();
    if (i == '1'){  
      mvprintw(18, 20, "Move 1: %s", current_def->get_move(0));
      mvprintw(19, 20, "damage: %d", current_def->get_move_power(0));
      if(current_def->get_move(1) != NULL){
        mvprintw(18, 40, "Move 2: %s", current_def->get_move(1));
        mvprintw(19, 40, "damage: %d", current_def->get_move_power(1));
      }
      int dmg;
      refresh();
      move(1,1);
      i = getch();
      int agg_moveidx = rand() % 2;

      if (i == '1'){
        if(current_def->get_move_priority(0) > current_agg->get_move_priority(agg_moveidx)){
          
          dmg = (((2 * current_def->level) / 5 + 2) * current_def->get_move_power(0) * (current_def->get_atk() / current_def->get_def())) / 50 + 2;
          mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                 ", current_def->get_species(), current_def->get_move(0), dmg);
          current_agg->hp -= dmg;
          if (current_agg->hp < 0){
            current_agg-> hp = 0;
          }
          mvprintw(6, 40, "        ");
          mvprintw(6, 40, "%d/%d", current_agg->hp, current_agg_mhp);
          move(1,1);
          getch();
          
          if (current_agg->hp <= 0){
            mvprintw(20, 1, "                                                                                  ");
            mvprintw(20, 1, "                                 %s has fainted!                                    ", current_agg->get_species());
            j = 0;
            for (i = 0; i < 6; ++i){
              if (aggressor->buddy[i] != NULL && aggressor->buddy[i]->hp > 0){
                j += 1;
              }
            }
            if (j == 0){
              init_battle_screen();
              mvprintw(10, 20, "YOU WIN, SEEK A POKECENTER");
              n->defeated = 1;
              if (n->ctype == char_hiker || n->ctype == char_rival) {
                n->mtype = move_wander;
              }
              getch();
              return;
            }
            *current_agg = choose_new_pokemon(aggressor);
            init_battle_screen();
            mvprintw(3, 45, "%s", current_agg->get_species());
            mvprintw(5, 48, ">*)");
            mvprintw(6, 48, "( \\");
            mvprintw(7, 49, "^");
            mvprintw(6, 40, "%d/%d", current_agg->hp, current_agg_mhp);

            mvprintw(11, 27, "(*<");
            mvprintw(12, 27, "/ )");
            mvprintw(13, 28, "L");
            mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
            mvprintw(14, 25, "%s", current_def->get_species());

              mvprintw( 3, 19, "Battle !!");
              
              
              mvprintw(16, 20, "1. Attack  2. Item  3. Run  4. Pokemon  ");
              mvprintw(17, 19, "                                         ");
              mvprintw(18, 19, "                                         ");
              mvprintw(19, 19, "                                         ");

            getch();
          }else{
            dmg = (((2 * current_agg->level) / 5 + 2) * current_agg->get_move_power(agg_moveidx) * (current_agg->get_atk() / current_agg->get_def())) / 50 + 2;
          mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                 ", current_agg->get_species(), current_agg->get_move(agg_moveidx), dmg);
          current_def->hp -= dmg;
          if (current_def->hp < 0){
            current_def-> hp = 0;
          }
          mvprintw(12, 32, "        ");
          mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
          move(1, 1);
          getch();
          
          }

          if (current_def->hp <= 0){
            mvprintw(20, 1, "                                                                                  ");
            mvprintw(20, 1, "                                 %s has fainted!                                    ", current_def->get_species());
            j = 0;
            for (i = 0; i < 6; ++i){
              if (defender->buddy[i] != NULL && defender->buddy[i]->hp > 0){
                j += 1;
              }
            }
            if (j == 0){
              init_battle_screen();
              mvprintw(10, 20, "YOU LOST, SEEK A POKECENTER");
              n->defeated = 1;
              if (n->ctype == char_hiker || n->ctype == char_rival) {
                n->mtype = move_wander;
              }
              getch();
              return;
            }
            *current_def = choose_new_pokemon(defender);
            getch();
          }
          mvprintw(20, 1, "                                 choose option                                    ");
        }
        else{
          dmg = (((2 * current_agg->level) / 5 + 2) * current_agg->get_move_power(agg_moveidx) * (current_agg->get_atk() / current_agg->get_def())) / 50 + 2;
          mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                  ", current_agg->get_species(), current_agg->get_move(agg_moveidx), dmg);
          current_def->hp -= dmg;
          if (current_def->hp < 0){
            current_def-> hp = 0;
          }
          mvprintw(12, 32, "        ");
          mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
          move(1,1);
          getch();
          if (current_def->hp <= 0){
            mvprintw(20, 1, "                                                                                  ");
            mvprintw(20, 1, "                                 %s has fainted!                                    ", current_def->get_species());
            j = 0;
            for (i = 0; i < 6; ++i){
              if (defender->buddy[i] != NULL && defender->buddy[i]->hp > 0){
                j += 1;
              }
            }
            if (j == 0){
              init_battle_screen();
              mvprintw(10, 20, "YOU LOST, SEEK A POKECENTER");
              n->defeated = 1;
              if (n->ctype == char_hiker || n->ctype == char_rival) {
                n->mtype = move_wander;
              }
              getch();
              return;
            }
            *current_def = choose_new_pokemon(defender);
            init_battle_screen();
            mvprintw(3, 45, "%s", current_agg->get_species());
            mvprintw(5, 48, ">*)");
            mvprintw(6, 48, "( \\");
            mvprintw(7, 49, "^");
            mvprintw(6, 40, "%d/%d", current_agg->hp, current_agg_mhp);

            mvprintw(11, 27, "(*<");
            mvprintw(12, 27, "/ )");
            mvprintw(13, 28, "L");
            mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
            mvprintw(14, 25, "%s", current_def->get_species());

              mvprintw( 3, 19, "Battle !!");
              
              
              mvprintw(16, 20, "1. Attack  2. Item  3. Run  4. Pokemon  ");
              mvprintw(17, 19, "                                         ");
              mvprintw(18, 19, "                                         ");
              mvprintw(19, 19, "                                         ");

            getch();
          }else{

          dmg = (((2 * current_def->level) / 5 + 2) * current_def->get_move_power(0) * (current_def->get_atk() / current_def->get_def())) / 50 + 2;
          mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                  ", current_def->get_species(), current_def->get_move(0), dmg);
          current_agg->hp -= dmg;
          if (current_agg->hp < 0){
            current_agg-> hp = 0;
          }
          mvprintw(6, 40, "        ");
          mvprintw(6, 40, "%d/%d", current_agg->hp, current_agg_mhp);
          move(1,1);
          getch();
          }

          if (current_agg->hp <= 0){
            mvprintw(20, 1, "                                                                                  ");
            mvprintw(20, 1, "                                 %s has fainted!                                    ", current_agg->get_species());
            j = 0;
            for (i = 0; i < 6; ++i){
              if (aggressor->buddy[i] != NULL && aggressor->buddy[i]->hp > 0){
                j += 1;
              }
            }
            if (j == 0){
              init_battle_screen();
              mvprintw(10, 20, "YOU WIN, SEEK A POKECENTER");
              n->defeated = 1;
              if (n->ctype == char_hiker || n->ctype == char_rival) {
                n->mtype = move_wander;
              }
              getch();
              return;
            }
            *current_agg = choose_new_pokemon(aggressor);
            getch();
          }
          mvprintw(20, 1, "                                 choose option                                    ");
        }
      }
      
      // int dmg;
      // refresh();
      // move(1,1);
      
      // int agg_moveidx = rand() % 2;
      

      if (i == '2'){
        if(current_def->get_move_priority(1) > current_agg->get_move_priority(agg_moveidx)){
          
          dmg = (((2 * current_def->level) / 5 + 2) * current_def->get_move_power(1) * (current_def->get_atk() / current_def->get_def())) / 50 + 2;
          mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                 ", current_def->get_species(), current_def->get_move(1), dmg);
          current_agg->hp -= dmg;
          mvprintw(6, 40, "        ");
          mvprintw(6, 40, "%d/%d", current_agg->hp, current_agg_mhp);
          move(1,1);
          getch();
          
          if (current_agg->hp <= 0){
            mvprintw(20, 1, "                                                                                  ");
            mvprintw(20, 1, "                                 %s has fainted!                                    ", current_agg->get_species());
            j = 0;
            for (i = 0; i < 6; ++i){
              if (aggressor->buddy[i] != NULL && aggressor->buddy[i]->hp > 0){
                j += 1;
              }
            }
            if (j == 0){
              init_battle_screen();
              mvprintw(10, 20, "YOU WIN, SEEK A POKECENTER");
              n->defeated = 1;
              if (n->ctype == char_hiker || n->ctype == char_rival) {
                n->mtype = move_wander;
              }
              getch();
              return;
            }
            *current_agg = choose_new_pokemon(aggressor);
            getch();
          }else{
            dmg = (((2 * current_agg->level) / 5 + 2) * current_agg->get_move_power(agg_moveidx) * (current_agg->get_atk() / current_agg->get_def())) / 50 + 2;
          mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                 ", current_agg->get_species(), current_agg->get_move(agg_moveidx), dmg);
          current_def->hp -= dmg;
          mvprintw(12, 32, "        ");
          mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
          move(1, 1);
          getch();
          
          }

          if (current_def->hp <= 0){
            mvprintw(20, 1, "                                                                                  ");
            mvprintw(20, 1, "                                 %s has fainted!                                    ", current_def->get_species());
            j = 0;
            for (i = 0; i < 6; ++i){
              if (defender->buddy[i] != NULL && defender->buddy[i]->hp > 0){
                j += 1;
              }
            }
            if (j == 0){
              init_battle_screen();
              mvprintw(10, 20, "YOU LOST, SEEK A POKECENTER");
              n->defeated = 1;
              if (n->ctype == char_hiker || n->ctype == char_rival) {
                n->mtype = move_wander;
              }
              getch();
              return;
            }
            *current_def = choose_new_pokemon(defender);
            getch();
          }
          mvprintw(20, 1, "                                 choose option                                    ");
        }
        else{
          dmg = (((2 * current_agg->level) / 5 + 2) * current_agg->get_move_power(agg_moveidx) * (current_agg->get_atk() / current_agg->get_def())) / 50 + 2;
          mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                  ", current_agg->get_species(), current_agg->get_move(agg_moveidx), dmg);
          current_def->hp -= dmg;
          mvprintw(12, 32, "        ");
          mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
          move(1,1);
          getch();
          if (current_def->hp <= 0){
            mvprintw(20, 1, "                                                                                  ");
            mvprintw(20, 1, "                                 %s has fainted!                                    ", current_def->get_species());
            j = 0;
            for (i = 0; i < 6; ++i){
              if (defender->buddy[i] != NULL && defender->buddy[i]->hp > 0){
                j += 1;
              }
            }
            if (j == 0){
              init_battle_screen();
              mvprintw(10, 20, "YOU LOST, SEEK A POKECENTER");
              n->defeated = 1;
              if (n->ctype == char_hiker || n->ctype == char_rival) {
                n->mtype = move_wander;
              }
              getch();
              return;
            }
            *current_def = choose_new_pokemon(defender);
            getch();
          }else{

          dmg = (((2 * current_def->level) / 5 + 2) * current_def->get_move_power(1) * (current_def->get_atk() / current_def->get_def())) / 50 + 2;
          mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                  ", current_def->get_species(), current_def->get_move(1), dmg);
          current_agg->hp -= dmg;
          mvprintw(6, 40, "        ");
          mvprintw(6, 40, "%d/%d", current_agg->hp, current_agg_mhp);
          move(1,1);
          getch();
          }

          if (current_agg->hp <= 0){
            mvprintw(20, 1, "                                                                                  ");
            mvprintw(20, 1, "                                 %s has fainted!                                    ", current_agg->get_species());
            j = 0;
            for (i = 0; i < 6; ++i){
              if (aggressor->buddy[i] != NULL && aggressor->buddy[i]->hp > 0){
                j += 1;
              }
            }
            if (j == 0){
              init_battle_screen();
              mvprintw(10, 20, "YOU WIN, SEEK A POKECENTER");
              n->defeated = 1;
              if (n->ctype == char_hiker || n->ctype == char_rival) {
                n->mtype = move_wander;
              }
              getch();
              return;
            }
            *current_agg = choose_new_pokemon(aggressor);
            getch();
          }
          mvprintw(20, 1, "                                 choose option                                    ");
        }
      }
    }
    if (i == '2'){
      init_battle_screen();
      mvprintw(8, 20, "Select an Item");
      mvprintw(9, 20, "1. Health Potions: %d", world.pc.health_item);
      mvprintw(10, 20, "2. Revives: %d", world.pc.revives);
      mvprintw(11, 20, "3.  Pokeball: %d", world.pc.pokeballs);
      bool selecting = true;
      while(selecting){

      
        i = getch();
        if (i == '1'){
          current_def->hp += 20;
          if (current_def->hp > current_def->get_hp()){
            current_def->hp = current_def->get_hp();
          }
          selecting = false;
          world.pc.health_item -= 1;
        }
        else if(i == '2'){
          init_battle_screen();
          mvprintw(8, 20, "Select a Pokemon");
          for (i = 0; i < 6; i++){
            if (defender->buddy[i] != NULL){
              mvprintw(9 + i, 20, "%d: %s,  %d/%d", i + 1, defender->buddy[i]->get_species(), defender->buddy[i]->hp, defender->buddy[i]->get_hp());
            }
          }
        
          i = getch();
          if (i == '1' && defender->buddy[0] != NULL){
            defender->buddy[0]->hp = defender->buddy[0]->get_hp();
            selecting = false;
          }
          else if (i == '2'){
            defender->buddy[1]->hp = defender->buddy[1]->get_hp();
            selecting = false;
          }
          else if (i == '3'){
            defender->buddy[2]->hp = defender->buddy[2]->get_hp();
            selecting = false;
          }
          else if (i == '4'){
            defender->buddy[3]->hp = defender->buddy[3]->get_hp();
            selecting = false;
          }
          else if (i == '5'){
            defender->buddy[4]->hp = defender->buddy[4]->get_hp();
            selecting = false;
          }
          else if (i == '6'){
            defender->buddy[5]->hp = defender->buddy[5]->get_hp();
            selecting = false;
          }
          else {
            
          }
          world.pc.revives -= 1;
        }
        else if (i == '3'){
          j = 0;
          if (aggressor->wild == true){
            for (i = 0; i < 6; i++){
              if (defender->buddy[i] != NULL){
                j+= 1;
              }
            }
            if (j != 6){
              defender->buddy[j] = current_agg;
              init_battle_screen();
              mvprintw(10, 20, "You Caught the wild %s", current_agg->get_species());
              world.pc.pokeballs -= 1;
              in_battle = false;
              selecting = false;
              getch();
            }
          }
          else {
            init_battle_screen();
            mvprintw(10, 20, "You cannot catch this pokemon");
            selecting = false;
            getch();
          }
          

        }
      }

      if (j == 0 || j == 6){
        int dmg;
        init_battle_screen();
        mvprintw(3, 45, "%s", current_agg->get_species());
        mvprintw(5, 48, ">*)");
        mvprintw(6, 48, "( \\");
        mvprintw(7, 49, "^");
        mvprintw(6, 40, "%d/%d", current_agg->hp, current_agg_mhp);

        mvprintw(11, 27, "(*<");
        mvprintw(12, 27, "/ )");
        mvprintw(13, 28, "L");
        mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
        mvprintw(14, 25, "%s", current_def->get_species());

          mvprintw( 3, 19, "Battle !!");
          
          
          mvprintw(16, 20, "1. Attack  2. Item  3. Run  4. Pokemon  ");
          mvprintw(17, 19, "                                         ");
          mvprintw(18, 19, "                                         ");
          mvprintw(19, 19, "                                         ");
        refresh();
        move(1,1);
        int agg_moveidx = rand() % 2;
          dmg = (((2 * current_agg->level) / 5 + 2) * current_agg->get_move_power(agg_moveidx) * (current_agg->get_atk() / current_agg->get_def())) / 50 + 2;
            mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                  ", current_agg->get_species(), current_agg->get_move(agg_moveidx), dmg);
            current_def->hp -= dmg;
            mvprintw(12, 32, "        ");
            mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
            move(1,1);
            getch();
      }

      
    }
    if (i == '3'){
      in_battle = false;
    }
    if (i == '4'){
      *current_def = choose_new_pokemon(defender);
      init_battle_screen();
      mvprintw(3, 45, "%s", current_agg->get_species());
        mvprintw(5, 48, ">*)");
        mvprintw(6, 48, "( \\");
        mvprintw(7, 49, "^");
        mvprintw(6, 40, "%d/%d", current_agg->hp, current_agg_mhp);

        mvprintw(11, 27, "(*<");
        mvprintw(12, 27, "/ )");
        mvprintw(13, 28, "L");
        mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
        mvprintw(14, 25, "%s", current_def->get_species());

          mvprintw( 3, 19, "Battle !!");
          
          
          mvprintw(16, 20, "1. Attack  2. Item  3. Run  4. Pokemon  ");
          mvprintw(17, 19, "                                         ");
          mvprintw(18, 19, "                                         ");
          mvprintw(19, 19, "                                         ");
      int dmg;
      refresh();
      move(1,1);
      int agg_moveidx = rand() % 2;
        dmg = (((2 * current_agg->level) / 5 + 2) * current_agg->get_move_power(agg_moveidx) * (current_agg->get_atk() / current_agg->get_def())) / 50 + 2;
          mvprintw(20, 1, "              %s attacks with %s, doing %d damage!                  ", current_agg->get_species(), current_agg->get_move(agg_moveidx), dmg);
          current_def->hp -= dmg;
          mvprintw(12, 32, "        ");
          mvprintw(12, 32, "%d/%d", current_def->hp, current_def_mhp);
          move(1,1);
          getch();
    }
  } while (in_battle);
  noecho();
  curs_set(0);

  n->defeated = 1;
  if (n->ctype == char_hiker || n->ctype == char_rival) {
    n->mtype = move_wander;
  }
}

void io_gym(){
  bool inGym = true;
  bool fightDone = false;

  curs_set(0);
  
  int x = 41;
  int y = 15;
  
    mvprintw( 4, 20, "---------------------------------------------");
    mvprintw( 5, 20, "|                                           |");
    mvprintw( 6, 20, "|                                           |");
    mvprintw( 7, 20, "|                                           |");
    mvprintw( 8, 20, "|                                           |");
    mvprintw( 9, 20, "|                                           |");
    mvprintw(10, 20, "|                                           |");
    mvprintw(11, 20, "|                                           |");
    mvprintw(12, 20, "|                                           |");
    mvprintw(13, 20, "|                                           |");
    mvprintw(14, 20, "|                                           |");
    mvprintw(15, 20, "|                                           |");
    mvprintw(16, 20, "----------------            -----------------");
    mvprintw(15, 41, "@");
    mvprintw(6, 41, "A");
    move(1,1);
   
  do {
    int key = getch();

    if (key == 's'){
      mvprintw(y,x,"  ");
      if ((y != 15) || (x > 35 && x < 48)){
        y+=1;
      }
      
      if (y>16 && x > 35 && x < 48){
        inGym = false;
      }
      mvprintw(y, x, "@");
      refresh();
      move(1,1);
    }
    else if (key == 'a'){
      mvprintw(y,x,"  ");
      if (x != 21){
        x-=1;
      }
      
      mvprintw(y, x, "@");
      refresh();
      move(1,1);
    }
    else if (key == 'w'){
      mvprintw(y,x,"  ");
      if (y != 5){
        y-=1;
      }
      mvprintw(y, x, "@");
      refresh();
      move(1,1);
    }

    else if (key == 'd'){
      mvprintw(y,x,"  ");
      if (x != 63){
        x+=1;
      }
      
      mvprintw(y, x, "@");
      refresh();
      move(1,1);
    }
    else if (key == 'q'){
      inGym = false;
    }


    if(((x == 41 && y == 7) || (x == 40 && y == 6) || (x == 42 && y == 6) || (x == 41 && y == 5)) && fightDone == false){
      npc *c = new_gym_trainer();
      io_battle(&world.pc, c);
      fightDone = true;
      mvprintw( 4, 20, "---------------------------------------------");
      mvprintw( 5, 20, "|                                           |");
      mvprintw( 6, 20, "|                                           |");
      mvprintw( 7, 20, "|                                           |");
      mvprintw( 8, 20, "|                                           |");
      mvprintw( 9, 20, "|                                           |");
      mvprintw(10, 20, "|                                           |");
      mvprintw(11, 20, "|                                           |");
      mvprintw(12, 20, "|                                           |");
      mvprintw(13, 20, "|                                           |");
      mvprintw(14, 20, "|                                           |");
      mvprintw(15, 20, "|                                           |");
      mvprintw(16, 20, "----------------            -----------------");
      mvprintw(8, 41, "@");
      mvprintw(6, 41, "A");
      move(1,1);
      x=41;
      y=8;
      curs_set(0);
      refresh();
    }
  } while (inGym);
}



uint32_t move_pc_dir(uint32_t input, pair_t dest)
{
  dest[dim_y] = world.pc.pos[dim_y];
  dest[dim_x] = world.pc.pos[dim_x];

  switch (input) {
  case 1:
  case 2:
  case 3:
    dest[dim_y]++;
    break;
  case 4:
  case 5:
  case 6:
    break;
  case 7:
  case 8:
  case 9:
    dest[dim_y]--;
    break;
  }
  switch (input) {
  case 1:
  case 4:
  case 7:
    dest[dim_x]--;
    break;
  case 2:
  case 5:
  case 8:
    break;
  case 3:
  case 6:
  case 9:
    dest[dim_x]++;
    break;
  case '>':
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_mart) {
      io_pokemart();
    }
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_center) {
      io_pokemon_center();
    }
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_gym) {
          io_gym();
        }
    break;
  }

  if (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) {
    if (dynamic_cast<npc *> (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) &&
        ((npc *) world.cur_map->cmap[dest[dim_y]][dest[dim_x]])->defeated) {
      // Some kind of greeting here would be nice
      return 1;
    } else if ((dynamic_cast<npc *>
                (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]))) {
      io_battle(&world.pc, world.cur_map->cmap[dest[dim_y]][dest[dim_x]]);
      // Not actually moving, so set dest back to PC position
      dest[dim_x] = world.pc.pos[dim_x];
      dest[dim_y] = world.pc.pos[dim_y];
    }
  }
  
  if (move_cost[char_pc][world.cur_map->map[dest[dim_y]][dest[dim_x]]] ==
      INT_MAX) {
    return 1;
  }

  return 0;
}

void io_teleport_world(pair_t dest)
{
  /* mvscanw documentation is unclear about return values.  I believe *
   * that the return value works the same way as scanf, but instead   *
   * of counting on that, we'll initialize x and y to out of bounds   *
   * values and accept their updates only if in range.                */
  int x = INT_MAX, y = INT_MAX;
  
  world.cur_map->cmap[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = NULL;

  echo();
  curs_set(1);
  do {
    mvprintw(0, 0, "Enter x [-200, 200]:           ");
    refresh();
    mvscanw(0, 21, "%d", &x);
  } while (x < -200 || x > 200);
  do {
    mvprintw(0, 0, "Enter y [-200, 200]:          ");
    refresh();
    mvscanw(0, 21, "%d", &y);
  } while (y < -200 || y > 200);

  refresh();
  noecho();
  curs_set(0);

  x += 200;
  y += 200;

  world.cur_idx[dim_x] = x;
  world.cur_idx[dim_y] = y;

  new_map(1);
  io_teleport_pc(dest);
}

void io_handle_input(pair_t dest)
{
  uint32_t turn_not_consumed;
  int key;

  do {
    switch (key = getch()) {
    case '7':
    case 'y':
    case KEY_HOME:
      turn_not_consumed = move_pc_dir(7, dest);
      break;
    case '8':
    case 'k':
    case KEY_UP:
      turn_not_consumed = move_pc_dir(8, dest);
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      turn_not_consumed = move_pc_dir(9, dest);
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      turn_not_consumed = move_pc_dir(6, dest);
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      turn_not_consumed = move_pc_dir(3, dest);
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      turn_not_consumed = move_pc_dir(2, dest);
      break;
    case '1':
    case 'b':
    case KEY_END:
      turn_not_consumed = move_pc_dir(1, dest);
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      turn_not_consumed = move_pc_dir(4, dest);
      break;
    case '5':
    case ' ':
    case '.':
    case KEY_B2:
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    case '>':
      turn_not_consumed = move_pc_dir('>', dest);
      break;
    case 'Q':
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      world.quit = 1;
      turn_not_consumed = 0;
      break;
      break;
    case 't':
      io_list_trainers();
      turn_not_consumed = 1;
      break;
    case 'p':
      /* Teleport the PC to a random place in the map.              */
      io_teleport_pc(dest);
      turn_not_consumed = 0;
      break;
    case 'f':
      /* Fly to any map in the world.                                */
      io_teleport_world(dest);
      turn_not_consumed = 0;
      break;    
    case 'q':
      /* Demonstrate use of the message queue.  You can use this for *
       * printf()-style debugging (though gdb is probably a better   *
       * option.  Not that it matters, but using this command will   *
       * waste a turn.  Set turn_not_consumed to 1 and you should be *
       * able to figure out why I did it that way.                   */
      io_queue_message("This is the first message.");
      io_queue_message("Since there are multiple messages, "
                       "you will see \"more\" prompts.");
      io_queue_message("You can use any key to advance through messages.");
      io_queue_message("Normal gameplay will not resume until the queue "
                       "is empty.");
      io_queue_message("Long lines will be truncated, not wrapped.");
      io_queue_message("io_queue_message() is variadic and handles "
                       "all printf() conversion specifiers.");
      io_queue_message("Did you see %s?", "what I did there");
      io_queue_message("When the last message is displayed, there will "
                       "be no \"more\" prompt.");
      io_queue_message("Have fun!  And happy printing!");
      io_queue_message("Oh!  And use 'Q' to quit!");

      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    default:
      /* Also not in the spec.  It's not always easy to figure out what *
       * key code corresponds with a given keystroke.  Print out any    *
       * unhandled key here.  Not only does it give a visual error      *
       * indicator, but it also gives an integer value that can be used *
       * for that key in this (or other) switch statements.  Printed in *
       * octal, with the leading zero, because ncurses.h lists codes in *
       * octal, thus allowing us to do reverse lookups.  If a key has a *
       * name defined in the header, you can use the name here, else    *
       * you can directly use the octal value.                          */
      mvprintw(0, 0, "Unbound key: %#o ", key);
      turn_not_consumed = 1;
    }
    refresh();
  } while (turn_not_consumed);
}

void io_encounter_pokemon()
{
  pokemon *p;

  p = new pokemon();
  p->hp = p->get_hp();
  init_battle_screen();

  character *aggresor = new character();
  aggresor->buddy[0] = p;
  aggresor->wild = true;

  io_battle(aggresor, &world.pc);



  // io_queue_message("%s%s%s: HP:%d ATK:%d DEF:%d SPATK:%d SPDEF:%d SPEED:%d %s",
  //                  p->is_shiny() ? "*" : "", p->get_species(),
  //                  p->is_shiny() ? "*" : "", p->get_hp(), p->get_atk(),
  //                  p->get_def(), p->get_spatk(), p->get_spdef(),
  //                  p->get_speed(), p->get_gender_string());
  // io_queue_message("%s's moves: %s %s", p->get_species(),
  //                  p->get_move(0), p->get_move(1));

  // Later on, don't delete if captured
}

void io_choose_starter()
{
  class pokemon *choice[3];
  int i;
  bool again = true;
  
  choice[0] = new class pokemon();
  choice[0]->hp = choice[0]->get_hp();
  choice[1] = new class pokemon();
  choice[1]->hp = choice[1]->get_hp();
  choice[2] = new class pokemon();
  choice[2]->hp = choice[2]->get_hp();
  echo();
  curs_set(1);
  do {
    mvprintw( 4, 20, "Before you are three Pokemon, each of");
    mvprintw( 5, 20, "which wants absolutely nothing more");
    mvprintw( 6, 20, "than to be your best buddy forever.");
    mvprintw( 8, 20, "Unfortunately for them, you may only");
    mvprintw( 9, 20, "pick one.  Choose wisely.");
    mvprintw(11, 20, "   1) %s", choice[0]->get_species());
    mvprintw(12, 20, "   2) %s", choice[1]->get_species());
    mvprintw(13, 20, "   3) %s", choice[2]->get_species());
    mvprintw(15, 20, "Enter 1, 2, or 3: ");

    refresh();
    i = getch();
    
    if (i == '1' || i == '2' || i == '3') {
      world.pc.buddy[0] = choice[(i - '0') - 1];
      delete choice[(i - '0') % 3];
      delete choice[((i - '0') + 1) % 3];
      again = false;
    }
  } while (again);
  noecho();
  curs_set(0);
}

