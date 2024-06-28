#ifndef MY_BOT_HELPER_H
#define MY_BOT_HELPER_H

#include <list>
#include <vector>

#include "hlt.hpp"
#include "networking.hpp"

const unsigned char FREE_ID = 0;
const unsigned char ALL_ID = 255;
// pentru id-urile casultelor ocupate de inamici
const unsigned char NOT_MINE_ID = 254;
unsigned char MY_ID;

inline void update_location(hlt::Location& loc, unsigned char y_coord, unsigned char x_coord) {
    loc.y = y_coord;
    loc.x = x_coord;
}

inline void make_move(std::set<hlt::Move>& moves, hlt::Location& loc, unsigned char move) {
    moves.insert({ loc, move });
}

inline unsigned short substract_y(hlt::GameMap& map, unsigned short y, unsigned short dy) {
    // we verify if we surpassed map bound
    if (y < dy) {
        return map.height - (dy - y);
    }

    return y - dy;
}

inline unsigned short substract_x(hlt::GameMap& map, unsigned short x, unsigned short dx) {
    // we verify if we surpassed map bound
    if (x < dx) {
        return map.width - (dx - x);
    }

    return x - dx;
}

inline hlt::Location substractLocations(hlt::GameMap map, hlt::Location l1, hlt::Location l2) {
    return {substract_x(map, l1.x, l2.x), substract_y(map, l1.y, l2.y)};
}

inline unsigned short add_y(hlt::GameMap& map, unsigned short y, unsigned short dy) {
    unsigned short sum = y + dy;
    // we verify if we surpassed map bound
    if (sum >= map.height) {
        return sum - map.height;
    }

    return sum;
}

inline unsigned short add_x(hlt::GameMap& map, unsigned short x, unsigned short dx) {
    unsigned short sum = x + dx;
    // we verify if we surpassed map bound
    if (sum >= map.width) {
        return sum - map.width;
    }

    return sum;
}

inline bool equalLocations(hlt::Location l1, hlt::Location l2) {
    return l1.x == l2.x && l1.y == l2.y;
}

// Functions to get in "range_loc" set all locations WITHIN a distance of "range" with one of "select id".
// If "select id" is empty, return all tiles within "range"
inline void get_locations_within_range(hlt::GameMap& map, hlt::Location& start_loc, unsigned short range, 
                                        unsigned char select_id, std::set<hlt::Location>& range_loc) {
    unsigned short i, y, x, actual_y, actual_x;
    unsigned short actual_y_dist = std::min(range, static_cast<unsigned short>(map.height / 2)); // <=> map.height / 2
    unsigned short actual_x_dist = std::min(range, static_cast<unsigned short>(map.width / 2));
    hlt::Location aux_locations[4];

    for (y = 0; y <= actual_y_dist; y++) {
        for (x = 0; x <= actual_x_dist; x++) {
            // we verify if we are beyond range
            if (x + y > range) {
                break;
            }

            // we add location in south-east
            actual_y = add_y(map, start_loc.y, y);
            actual_x = add_x(map, start_loc.x, x);
            update_location(aux_locations[0], actual_y, actual_x);

            // we add location in south-west
            actual_x = substract_x(map, start_loc.x, x);
            update_location(aux_locations[1], actual_y, actual_x);

            // we add location in north-west
            actual_y = substract_y(map, start_loc.y, y);
            update_location(aux_locations[2], actual_y, actual_x);

            // we add location in north-east
            actual_x = add_x(map, start_loc.x, x);
            update_location(aux_locations[3], actual_y, actual_x);

            for (i = 0; i < 4; i++) {
                switch (select_id) {
                    case ALL_ID: range_loc.insert(aux_locations[i]); break;
                    case NOT_MINE_ID:
                        if (map.getSite(aux_locations[i]).owner != MY_ID) {
                            range_loc.insert(aux_locations[i]);
                        }
                        break;
                    default:
                        if (map.getSite(aux_locations[i]).owner == select_id) {
                            range_loc.insert(aux_locations[i]);
                        }
                }
            }
        }
    }
}

// Functions to get in "range_loc" set all locations AT a distance of EXACTLY "range" with one of "select id".
// If "select id" is empty, return all tiles within "range"
inline void get_locations_at_range(hlt::GameMap& map, hlt::Location& start_loc, unsigned short range, 
                                        unsigned char select_id, std::set<hlt::Location>& range_loc) {
    unsigned short i, y, x, actual_y, actual_x;
    unsigned short actual_y_dist = std::min(range, static_cast<unsigned short>(map.height / 2)); // <=> map.height / 2
    unsigned short actual_x_dist = std::min(range, static_cast<unsigned short>(map.width / 2));
    unsigned short actual_range = std::min(actual_x_dist, actual_y_dist);
    hlt::Location aux_locations[4];

    for (y = 0; y <= actual_range; y++) {
        x = actual_range - y;
        // we add location in south-east
        actual_y = add_y(map, start_loc.y, y);
        actual_x = add_x(map, start_loc.x, x);
        update_location(aux_locations[0], actual_y, actual_x);

        // we add location in south-west
        actual_x = substract_x(map, start_loc.x, x);
        update_location(aux_locations[1], actual_y, actual_x);

        // we add location in north-west
        actual_y = substract_y(map, start_loc.y, y);
        update_location(aux_locations[2], actual_y, actual_x);

        // we add location in north-east
        actual_x = add_x(map, start_loc.x, x);
        update_location(aux_locations[3], actual_y, actual_x);

        for (i = 0; i < 4; i++) {
            switch (select_id) {
                case ALL_ID: range_loc.insert(aux_locations[i]); break;
                case NOT_MINE_ID:
                    if (map.getSite(aux_locations[i]).owner != MY_ID) {
                        range_loc.insert(aux_locations[i]);
                    }
                    break;
                default:
                    if (map.getSite(aux_locations[i]).owner == select_id) {
                        range_loc.insert(aux_locations[i]);
                    }
            }
        }
    }
}

inline bool is_in_range(hlt::GameMap map, hlt::Location area_pos, unsigned short width, unsigned short height, hlt::Location pos) {
    if (area_pos.x + width > map.width) {
        if (pos.x < area_pos.x && pos.x > area_pos.x + width - map.width) {
            return false;
        }
    } else {
        if (pos.x < area_pos.x || pos.x > area_pos.x + width) {
            return false;
        }
    }
    if (area_pos.y + height > map.height) {
        if (pos.y < area_pos.y && pos.y > area_pos.y + height - map.height) {
            return false;
        }
    } else {
        if (pos.y < area_pos.y || pos.y > area_pos.y + height) {
            return false;
        }
    }
    return true;
}

typedef struct MoveData {
    hlt::Location location;
    const int move;
} MoveData;

#endif