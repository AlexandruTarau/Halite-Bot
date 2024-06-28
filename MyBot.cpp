#include <stdlib.h>
#include <time.h>
#include <cstdlib>
#include <ctime>
#include <time.h>
#include <set>
#include <fstream>

#include "hlt.hpp"
#include "networking.hpp"
#include "MyBotHelper.h"
#include <iomanip>
#include <unordered_set>

const float PROD_PERCENT = 0.6;
const float STR_PERCENT = 0.4;

struct SquareData {
    float value;
    hlt::Location pos;
};

struct Ally {
    hlt::Location pos;
    std::vector<hlt::Move> moves;
    std::vector<Ally> providers;  // For conquerers only

    bool operator==(const Ally& other) const {
        if (pos.x == other.pos.x && pos.y == other.pos.y)
            return true;
        return false;
    };

    bool operator<(const Ally& other) {
        return hlt::operator<(pos, other.pos);
    };

    size_t operator()(const Ally& allyToHash) const noexcept {
        size_t hash = pos.x + 10 * pos.y;
        return hash;
    };
};

template<> struct std::hash<Ally> {
    size_t operator()(const Ally& ally) const {
        return ally(ally);
    }
};

unsigned char findClosestNonTerritory(hlt::GameMap map, hlt::Location pos, unsigned char id) {
    int minDistance = std::numeric_limits<int>::max();
    unsigned char closestDirection = 0;
    hlt::Location aux = pos;

    // Search NORTH
    for (int i = pos.y - 1; i >= 0; --i) {
        hlt::Location new_pos = map.getLocation(aux, NORTH);
        hlt::Site new_site = map.contents[new_pos.y][new_pos.x];
        if (new_site.owner != id) {
            int dist = pos.y - new_pos.y;
            if (dist < minDistance) {
                minDistance = dist;
                closestDirection = NORTH;
            }
            break;
        }
        aux = new_pos;
    }
    aux = pos;

    // Search EAST
    for (int i = pos.x + 1; i < map.width; ++i) {
        hlt::Location new_pos = map.getLocation(aux, EAST);
        hlt::Site new_site = map.contents[new_pos.y][new_pos.x];
        if (new_site.owner != id) {
            int dist = new_pos.x - pos.x;
            if (dist < minDistance) {
                minDistance = dist;
                closestDirection = EAST;
            }
            break;
        }
        aux = new_pos;
    }
    aux = pos;

    // Search SOUTH
    for (int i = pos.y + 1; i < map.height; ++i) {
        hlt::Location new_pos = map.getLocation(aux, SOUTH);
        hlt::Site new_site = map.contents[new_pos.y][new_pos.x];
        if (new_site.owner != id) {
            int dist = new_pos.y - pos.y;
            if (dist < minDistance) {
                minDistance = dist;
                closestDirection = SOUTH;
            }
            break;
        }
        aux = new_pos;
    }
    aux = pos;

    // Search WEST
    for (int i = pos.x - 1; i >= 0; --i) {
        hlt::Location new_pos = map.getLocation(aux, WEST);
        hlt::Site new_site = map.contents[new_pos.y][new_pos.x];
        if (new_site.owner != id) {
            int dist = pos.x - new_pos.x;
            if (dist < minDistance) {
                minDistance = dist;
                closestDirection = WEST;
            }
            break;
        }
        aux = new_pos;
    }

    return closestDirection;
}

float calculateValue(unsigned char production, unsigned char strength) {
    return (production * PROD_PERCENT) / (production * PROD_PERCENT + strength * STR_PERCENT);
}

float calculateAverageValue(hlt::GameMap map, hlt::Location pos, unsigned short width, unsigned short height) {
    float sum = 0.0f;
    for (unsigned short y = 0; y < height; ++y) {
        for (unsigned short x = 0; x < width; ++x) {
            hlt::Site site = map.getSite({add_x(map, x, pos.x), add_y(map, y, pos.y)});
            if (site.owner != MY_ID) {
                sum += calculateValue(site.production, site.strength);
            }
        }
    }
    return sum / (width * height);
}

SquareData findBestAreaRecursively(hlt::GameMap map, hlt::Location pos, unsigned short width, unsigned short height) {
    if (width * height == 0) {
        return {-1.0f, pos};
    }
    if (width == 1 && height == 1) {
        hlt::Site site = map.getSite({pos.x, pos.y});
        return {calculateValue(site.production, site.strength), pos};
    }

    unsigned short halfWidth = width / 2;
    unsigned short halfHeight = height / 2;

    float maxAverageValue = -1.0f;
    hlt::Location bestAreaPos, locations[4];
    locations[0] = pos;
    locations[1] = {add_x(map, pos.x, halfWidth), pos.y},
    locations[2] = {pos.x, add_y(map, pos.y, halfHeight)},
    locations[3] = {add_x(map, pos.x, halfWidth), add_y(map, pos.y, halfHeight)};

    for (int i = 0; i < 4; i++) {
        float val = calculateAverageValue(map, locations[i], halfWidth, halfHeight);
        if (maxAverageValue < val) {
            maxAverageValue = val;
            bestAreaPos = locations[i];
        }
    }
    if (maxAverageValue == 0.0f) {
        hlt::Site site = map.getSite({pos.x, pos.y});
        return {calculateValue(site.production, site.strength), pos};
    }

    return findBestAreaRecursively(map, bestAreaPos, halfWidth, halfHeight);
}

SquareData findBestArea(hlt::GameMap map) {
    return findBestAreaRecursively(map, {0, 0}, map.width, map.height);
}

struct Node {
    hlt::Location location;
    float cost;
};

bool operator<(const Node& a, const Node& b) {
    return a.cost > b.cost; // For min-heap
}

std::vector<hlt::Move> bestPathToConquer(hlt::GameMap map, hlt::Location start, hlt::Location dest) {
    std::vector<std::vector<float>> dist(map.height, std::vector<float>(map.width, std::numeric_limits<float>::max()));
    std::vector<std::vector<hlt::Location>> parent(map.height, std::vector<hlt::Location>(map.width));
    std::vector<std::vector<unsigned char>> directions(map.height, std::vector<unsigned char>(map.width));

    dist[start.y][start.x] = 0.0f;
    std::vector<Node> nodes;
    nodes.push_back({start, 0.0f});

    int dx[] = {-1, 1, 0, 0};
    int dy[] = {0, 0, -1, 1};

    while (!nodes.empty()) {
        // Sort nodes based on cost
        std::sort(nodes.begin(), nodes.end());

        Node current = nodes.back();
        nodes.pop_back();

        if (current.location.x == dest.x && current.location.y == dest.y)
            break;

        for (int i = 0; i < 4; ++i) {
            unsigned short nx = current.location.x + dx[i];
            unsigned short ny = current.location.y + dy[i];
            if (nx < 0) {
                nx = map.width - 1;
            } else if (nx >= map.width) {
                nx = 0;
            }
            if (ny < 0) {
                ny = map.height - 1;
            } else if (ny >= map.height) {
                ny = 0;
            }

            float newCost = current.cost + (1 - calculateValue(map.getSite({nx, ny}).production, map.getSite({nx, ny}).strength));
            if (newCost < dist[ny][nx]) {
                dist[ny][nx] = newCost;
                parent[ny][nx] = current.location;
                if (dx[i] == -1) {  // WEST
                    directions[ny][nx] = WEST;
                } else if (dx[i] == 1) {  // EAST
                    directions[ny][nx] = EAST;
                } else if (dy[i] == -1) {  // NORTH
                    directions[ny][nx] = NORTH;
                } else {  // SOUTH
                    directions[ny][nx] = SOUTH;
                }
                
                nodes.push_back({{nx, ny}, newCost});
            }
        }
    }

    // Reconstruct path from destination to start
    std::vector<hlt::Move> path;
    hlt::Location current = dest;
    unsigned char dir;
    while (!(current.x == start.x && current.y == start.y)) {
        dir = directions[current.y][current.x];
        current = parent[current.y][current.x];
        path.push_back({current, dir});
    }
    //path.push_back({start, directions[start.y][start.x]});
    //std::reverse(path.begin(), path.end());
    
    return path;
}

std::vector<hlt::Move> bestPathToProvide(hlt::GameMap map, hlt::Location start, std::unordered_set<Ally> conquerers) {
    int minDist = std::numeric_limits<int>::max();
    std::vector<hlt::Move> minPath;

    for (Ally conquerer : conquerers) {
        std::vector<std::vector<int>> dist(map.height, std::vector<int>(map.width, std::numeric_limits<int>::max()));
        std::vector<std::vector<hlt::Location>> parent(map.height, std::vector<hlt::Location>(map.width));
        std::vector<std::vector<unsigned char>> directions(map.height, std::vector<unsigned char>(map.width));

        dist[start.y][start.x] = 0;
        std::vector<Node> nodes;
        nodes.push_back({start, 0});

        int dx[] = {-1, 1, 0, 0};
        int dy[] = {0, 0, -1, 1};

        while (!nodes.empty()) {
            // Sort nodes based on cost
            std::sort(nodes.begin(), nodes.end());

            Node current = nodes.back();
            nodes.pop_back();

            if (current.location.x == conquerer.pos.x && current.location.y == conquerer.pos.y)
                break;

            for (int i = 0; i < 4; ++i) {
                unsigned short nx = current.location.x + dx[i];
                unsigned short ny = current.location.y + dy[i];
                if (nx < 0) {
                    nx = map.width - 1;
                } else if (nx >= map.width) {
                    nx = 0;
                }
                if (ny < 0) {
                    ny = map.height - 1;
                } else if (ny >= map.height) {
                    ny = 0;
                }

                if (map.getSite({nx, ny}).owner != MY_ID) {
                    continue;
                }

                int newCost = current.cost + 1;
                if (newCost < dist[ny][nx]) {
                    dist[ny][nx] = newCost;
                    parent[ny][nx] = current.location;
                    if (dx[i] == -1) {  // WEST
                        directions[ny][nx] = WEST;
                    } else if (dx[i] == 1) {  // EAST
                        directions[ny][nx] = EAST;
                    } else if (dy[i] == -1) {  // NORTH
                        directions[ny][nx] = NORTH;
                    } else {  // SOUTH
                        directions[ny][nx] = SOUTH;
                    }
                    
                    nodes.push_back({{nx, ny}, (float)newCost});
                }
            }
        }

        // Reconstruct path from destination to start
        std::vector<hlt::Move> path;
        hlt::Location current = conquerer.pos;
        unsigned char dir;
        while (!(current.x == start.x && current.y == start.y)) {
            dir = directions[current.y][current.x];
            current = parent[current.y][current.x];
            path.push_back({current, dir});
        }
        int distance = path.size();
        if (distance < minDist) {
            minDist = distance;
            minPath = path;
        }
    }
    return minPath;
}

bool isConqueror(hlt::GameMap map, Ally ally) {
    return map.getSite(ally.pos, NORTH).owner != MY_ID ||
           map.getSite(ally.pos, EAST).owner != MY_ID ||
           map.getSite(ally.pos, SOUTH).owner != MY_ID ||
           map.getSite(ally.pos, WEST).owner != MY_ID;
}

void insertFreelancer(hlt::GameMap presentMap, std::vector<Ally> conquerors,
                Ally freelancer, std::unordered_set<Ally> &new_freelancers, int dir) {
    int ok = 1;
    for (Ally conqueror : conquerors) {
        if (equalLocations(presentMap.getLocation(freelancer.pos, dir), conqueror.pos)) {
            ok = 0;
            break;
        }
    }
    if (ok) {
        new_freelancers.insert({presentMap.getLocation(freelancer.pos, dir)});
    }
}

int main() {
    srand(time(NULL));

    std::cout.sync_with_stdio(0);

    unsigned char myID;
    hlt::GameMap presentMap;
    getInit(myID, presentMap);
    sendInit("MyC++Bot");

    MY_ID = myID;

    std::set<hlt::Move> moves;

    hlt::Location myLocation;
    hlt::Site mySite;
    bool done = false;
    for(unsigned short a = 0; a < presentMap.height; a++) {
        for(unsigned short b = 0; b < presentMap.width; b++) {
            hlt::Site site = presentMap.getSite({ b, a });
            if (site.owner == myID) {
                mySite = site;
                myLocation = {b, a};
                done = true;
                break;
            }
        }
        if (done) {
            break;
        }
    }
    unsigned short range_multiplier = 2;
    unsigned short resizedWidth = presentMap.width < 4 ? std::min(presentMap.width, presentMap.height) : 4;
    unsigned short resizedHeight = presentMap.height < 4 ? std::min(presentMap.width, presentMap.height) : 4;

    hlt::Location areaLocation = substractLocations(presentMap, myLocation,
                                    {static_cast<unsigned short>(resizedWidth / 2),
                                    static_cast<unsigned short>(resizedHeight / 2)});
    hlt::Location bestLocation = findBestAreaRecursively(presentMap, areaLocation,
                                        resizedWidth, resizedHeight).pos;
    std::vector<hlt::Move> shortestPath = bestPathToConquer(presentMap, myLocation, bestLocation);

    std::vector<Ally> conquerors;
    std::unordered_set<Ally> providers;
    std::unordered_set<Ally> freelancers;
    std::unordered_set<Ally> new_freelancers;
    
    conquerors.push_back({myLocation, shortestPath});
    int turn = 0;
    bool ok = 0;
    int max_conquerors = 5;
    int nr_conquerors = 1;

    while(true) {   // GAME STARTS HERE 
        moves.clear();

        getFrame(presentMap);

        if (nr_conquerors < max_conquerors) {
            for (auto it = freelancers.begin(); it != freelancers.end();) {
                bool added_conq = false;
                Ally freelancer = *it;
                hlt::Site center = presentMap.getSite(freelancer.pos);
                if (center.owner == MY_ID) {
                    hlt::Location north_pos = { freelancer.pos.x, static_cast<unsigned short>(freelancer.pos.y - 1 < 0 ? presentMap.height - 1 : freelancer.pos.y - 1) };
                    hlt::Location east_pos = { static_cast<unsigned short>(freelancer.pos.x + 1 > presentMap.width - 1 ? 0 : freelancer.pos.x + 1), freelancer.pos.y };
                    hlt::Location south_pos = { freelancer.pos.x, static_cast<unsigned short>(freelancer.pos.y + 1 > presentMap.height - 1 ? 0 : freelancer.pos.y + 1) };
                    hlt::Location west_pos = { static_cast<unsigned short>(freelancer.pos.x - 1 < 0 ? presentMap.width - 1 : freelancer.pos.x - 1), freelancer.pos.y };
                    hlt::Site north = presentMap.getSite(north_pos);
                    hlt::Site east = presentMap.getSite(east_pos);
                    hlt::Site south = presentMap.getSite(south_pos);
                    hlt::Site west = presentMap.getSite(west_pos);

                    if (north.owner != myID || east.owner != myID || south.owner != myID || west.owner != myID) {
                        for (Ally conqueror : conquerors) {
                            if (!is_in_range(presentMap,
                                            substractLocations(presentMap, conqueror.pos,
                                                    {static_cast<unsigned short>(presentMap.width / 10),
                                                    static_cast<unsigned short>(presentMap.height / 10)}),
                                            presentMap.width / 5,
                                            presentMap.height / 5,
                                            freelancer.pos
                                            )) {
                                areaLocation = substractLocations(presentMap, freelancer.pos,
                                            {static_cast<unsigned short>(resizedWidth / 2),
                                            static_cast<unsigned short>(resizedHeight / 2)});
                                bestLocation = findBestAreaRecursively(presentMap, areaLocation,
                                                                    resizedWidth, resizedHeight).pos;
                                freelancer.moves = bestPathToConquer(presentMap, freelancer.pos, bestLocation);

                                conquerors.push_back(freelancer);
                                it = freelancers.erase(it);
                                nr_conquerors++;
                                added_conq = true;
                                break;
                            }
                        }
                    }
                }
                if (!added_conq) {
                    ++it;
                }
            }
        }
        
        for (auto it = conquerors.begin(); it != conquerors.end();) {
            Ally& conqueror = *it;
            if (presentMap.getSite(conqueror.pos).owner != MY_ID) {
                bool found_new_conq = 0;
                for (auto itp = conqueror.providers.rbegin(); itp != conqueror.providers.rend(); ++itp) {
                    Ally& provider = *itp;
                    hlt::Site current = presentMap.getSite(provider.pos);

                    if (current.owner == MY_ID) {
                        conqueror.pos = provider.pos;
                        conqueror.moves = provider.moves;
                        conqueror.providers = std::vector<Ally>(conqueror.providers.begin(), itp.base() - 1);
                        found_new_conq = 1;
                        break;
                    }
                }

                if (!found_new_conq) {
                    it = conquerors.erase(it);
                    nr_conquerors--;
                    continue;
                }
            }

            // The point has been conquered
            if (ok || presentMap.getSite(conqueror.moves[0].loc, conqueror.moves[0].dir).owner == MY_ID) {
                freelancers.insert(conqueror.providers.begin(), conqueror.providers.end());
                conqueror.providers.clear();

                areaLocation = substractLocations(presentMap, conqueror.pos,
                            {static_cast<unsigned short>(resizedWidth / 2),
                            static_cast<unsigned short>(resizedHeight / 2)});
                bestLocation = findBestAreaRecursively(presentMap, areaLocation,
                                                    resizedWidth, resizedHeight).pos;
                conqueror.moves = bestPathToConquer(presentMap, conqueror.pos, bestLocation);
                ok = 0;
            }

            if (!conqueror.moves.empty()) {
                hlt::Move move = conqueror.moves.back();
                hlt::Location next_location = presentMap.getLocation(move.loc, move.dir);
                hlt::Site current_site = presentMap.getSite(move.loc);
                hlt::Site next_site = presentMap.getSite(next_location);

                // Provide the conqueror
                int current_production = 0;
                for (Ally provider : conqueror.providers) {
                    hlt::Site current = presentMap.getSite(provider.pos);
                    current_production += current.strength;
                }
                for (auto itp = conqueror.providers.rbegin(); itp != conqueror.providers.rend(); ++itp) {
                    Ally& provider = *itp;
                    hlt::Site current = presentMap.getSite(provider.pos);

                    if (current.owner == MY_ID) {
                        if (current_production + current_site.strength > next_site.strength) {
                            moves.insert({provider.moves.back()});
                        } else {
                            moves.insert({provider.moves.back().loc, STILL});
                            current_production += current.production;
                        }
                    }
                }

                if (current_site.strength > next_site.strength) {
                    moves.insert(move);

                    conqueror.providers.push_back({move.loc, conqueror.moves});
                    conqueror.moves.pop_back();
                    conqueror.pos.x = next_location.x; conqueror.pos.y = next_location.y;

                    if (conqueror.moves.empty()) {
                        ok = 1;
                    }
                } else {
                    moves.insert({move.loc, STILL});
                }
            } else {
                ok = 1;
                unsigned short added_range = resizedHeight + range_multiplier;
                unsigned short min_dim = std::min(presentMap.height, presentMap.width);
                unsigned short new_range = std::min(min_dim, added_range);
                resizedHeight = new_range;
                resizedWidth = new_range;
            }
            it++;
        }

        for (auto it = freelancers.begin(); it != freelancers.end();) {
            Ally freelancer = *it;
            hlt::Site center = presentMap.getSite(freelancer.pos);
            if (center.owner == MY_ID) {
                int bestChoice = 0;
                hlt::Location north_pos = { freelancer.pos.x, static_cast<unsigned short>(freelancer.pos.y - 1 < 0 ? presentMap.height - 1 : freelancer.pos.y - 1) };
                hlt::Location east_pos = { static_cast<unsigned short>(freelancer.pos.x + 1 > presentMap.width - 1 ? 0 : freelancer.pos.x + 1), freelancer.pos.y };
                hlt::Location south_pos = { freelancer.pos.x, static_cast<unsigned short>(freelancer.pos.y + 1 > presentMap.height - 1 ? 0 : freelancer.pos.y + 1) };
                hlt::Location west_pos = { static_cast<unsigned short>(freelancer.pos.x - 1 < 0 ? presentMap.width - 1 : freelancer.pos.x - 1), freelancer.pos.y };
                hlt::Site north = presentMap.getSite(north_pos);
                hlt::Site east = presentMap.getSite(east_pos);
                hlt::Site south = presentMap.getSite(south_pos);
                hlt::Site west = presentMap.getSite(west_pos);

                float ratio, maxRatio = 0;
                float ratio_percent = 0.6, edge_percent = 0.4;
                if (north.owner != myID) {
                    ratio = calculateValue(north.production, north.strength);
                    ratio = (ratio * ratio_percent) / (ratio * ratio_percent + north_pos.y * edge_percent);
                    if (maxRatio < ratio) {
                        maxRatio = ratio;
                        bestChoice = 1;
                    }
                }
                if (east.owner != myID) {
                    ratio = calculateValue(east.production, east.strength);
                    ratio = (ratio * ratio_percent) / (ratio * ratio_percent + (presentMap.width - east_pos.x) * edge_percent);
                    if (maxRatio < ratio) {
                        maxRatio = ratio;
                        bestChoice = 2;
                    }
                }
                if (south.owner != myID) {
                    ratio = calculateValue(south.production, south.strength);
                    ratio = (ratio * ratio_percent) / (ratio * ratio_percent + (presentMap.height - south_pos.y) * edge_percent);
                    if (maxRatio < ratio) {
                        maxRatio = ratio;
                        bestChoice = 3;
                    }
                }
                if (west.owner != myID) {
                    ratio = calculateValue(west.production, west.strength);
                    ratio = (ratio * ratio_percent) / (ratio * ratio_percent + west_pos.x * edge_percent);
                    if (maxRatio < ratio) {
                        maxRatio = ratio;
                        bestChoice = 4;
                    }
                }

                switch (bestChoice) {
                    case 1: {
                        if (north.strength < center.strength) {
                            moves.insert({ freelancer.pos, NORTH});
                            insertFreelancer(presentMap, conquerors, freelancer, new_freelancers, NORTH);
                        }
                        break;
                    }
                    case 2: {
                        if (east.strength < center.strength) {
                            moves.insert({ freelancer.pos, EAST});
                            insertFreelancer(presentMap, conquerors, freelancer, new_freelancers, EAST);
                        }
                        break;
                    }
                    case 3: {
                        if (south.strength < center.strength) {
                            moves.insert({ freelancer.pos, SOUTH});
                            insertFreelancer(presentMap, conquerors, freelancer, new_freelancers, SOUTH);
                        }
                        break;
                    }
                    case 4: {
                        if (west.strength < center.strength) {
                            moves.insert({ freelancer.pos, WEST});
                            insertFreelancer(presentMap, conquerors, freelancer, new_freelancers, WEST);
                        }
                        break;
                    }
                    default: {
                        if (center.strength >= 20) {
                            unsigned char dir = findClosestNonTerritory(presentMap, freelancer.pos, myID);
                            moves.insert({ freelancer.pos, dir});
                            insertFreelancer(presentMap, conquerors, freelancer, new_freelancers, dir);
                        } else {
                            moves.insert({ freelancer.pos, STILL});
                        }
                        break;
                    }
                }
                it++;
            } else {
                it = freelancers.erase(it);
            }
        }
        freelancers.insert(new_freelancers.begin(), new_freelancers.end());
        new_freelancers.clear();

        sendFrame(moves);
    }
    return 0;
}
