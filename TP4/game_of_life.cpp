/*
 * Le jeu de la vie
 * ################
 * Le jeu de la vie est un automate cellulaire inventé par Conway se basant normalement sur une grille infinie
 * de cellules en deux dimensions. Ces cellules peuvent prendre deux états :
 *     - un état vivant
 *     - un état mort
 * A l'initialisation, certaines cellules sont vivantes, d'autres mortes.
 * Le principe du jeu est alors d'itérer de telle sorte qu'à chaque itération, une cellule va devoir interagir avec
 * les huit cellules voisines (gauche, droite, bas, haut et les quatre en diagonales.) L'interaction se fait selon les
 * règles suivantes pour calculer l'irération suivante :
 *     - Une cellule vivante avec moins de deux cellules voisines vivantes meurt ( sous-population )
 *     - Une cellule vivante avec deux ou trois cellules voisines vivantes reste vivante
 *     - Une cellule vivante avec plus de trois cellules voisines vivantes meurt ( sur-population )
 *     - Une cellule morte avec exactement trois cellules voisines vivantes devient vivante ( reproduction )
 *
 * Pour ce projet, on change légèrement les règles en transformant la grille infinie en un tore contenant un
 * nombre fini de cellules. Les cellules les plus à gauche ont pour voisines les cellules les plus à droite
 * et inversement, et de même les cellules les plus en haut ont pour voisines les cellules les plus en bas
 * et inversement.
 *
 * On itère ensuite pour étudier la façon dont évolue la population des cellules sur la grille.
 */

#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>
#include <tuple>
#include <mpi.h>
#include <SDL2/SDL.h>

// Type for a cell position
using Position = std::pair<int, int>;
// Type for a pattern (list of positions)
using Pattern = std::vector<Position>;

class Grille {
public:
    Grille(int rank, int nbp, std::pair<int, int> dim, const Pattern* init_pattern = nullptr,
           SDL_Color color_life = {0, 0, 0, 255}, SDL_Color color_dead = {255, 255, 255, 255}) 
        : dimensions(dim), col_life(color_life), col_dead(color_dead) {
        
        // Calculate local dimensions for this process
        dimensions_loc.first = dim.first / nbp + (rank < dim.first % nbp ? 1 : 0);
        dimensions_loc.second = dim.second;
        
        // Calculate starting position for this process
        start_loc = rank * (dim.first / nbp) + (rank >= dim.first % nbp ? dim.first % nbp : rank);
        
        // Initialize cells with ghost cells (+2 in first dimension)
        cells.resize(dimensions_loc.first + 2);
        for (auto& row : cells) {
            row.resize(dimensions_loc.second, 0);
        }
        
        if (init_pattern != nullptr) {
            // Set initial pattern
            for (const auto& pos : *init_pattern) {
                int i = pos.first - start_loc + 1;
                int j = pos.second;
                if (i >= 1 && i <= dimensions_loc.first) {
                    cells[i][j] = 1;
                }
            }
        } else {
            // Random initialization
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distrib(0, 1);
            for (int i = 1; i <= dimensions_loc.first; ++i) {
                for (int j = 0; j < dimensions_loc.second; ++j) {
                    cells[i][j] = distrib(gen);
                }
            }
        }
    }
    
    void compute_next_iteration() {
        std::vector<std::vector<unsigned char>> next_cells = cells;
        
        // For each cell in the grid (excluding ghost cells)
        for (int i = 1; i <= dimensions_loc.first; ++i) {
            for (int j = 0; j < dimensions_loc.second; ++j) {
                int neighbors_count = 0;
                
                // Count neighbors (including wraparound)
                for (int di = -1; di <= 1; ++di) {
                    for (int dj = -1; dj <= 1; ++dj) {
                        if (di == 0 && dj == 0) continue;
                        
                        int ni = i + di;
                        int nj = (j + dj + dimensions_loc.second) % dimensions_loc.second;
                        
                        if (ni >= 0 && ni < cells.size()) {
                            neighbors_count += cells[ni][nj];
                        }
                    }
                }
                
                // Apply Game of Life rules
                if (cells[i][j] == 1) {
                    if (neighbors_count < 2 || neighbors_count > 3) {
                        next_cells[i][j] = 0;  // Death
                    }
                } else {
                    if (neighbors_count == 3) {
                        next_cells[i][j] = 1;  // Birth
                    }
                }
            }
        }
        
        cells = next_cells;
    }
    
    void update_ghost_cells(MPI_Comm comm) {
        int rank = 0, size = 0;
        MPI_Comm_rank(comm, &rank);
        MPI_Comm_size(comm, &size);
        
        // Send bottom row to next process, receive into top ghost row
        MPI_Request req1, req2;
        MPI_Irecv(&cells[0][0], dimensions_loc.second, MPI_UNSIGNED_CHAR, 
                 (rank + size - 1) % size, 102, comm, &req1);
        MPI_Irecv(&cells[dimensions_loc.first + 1][0], dimensions_loc.second, MPI_UNSIGNED_CHAR, 
                 (rank + 1) % size, 101, comm, &req2);
        
        MPI_Send(&cells[1][0], dimensions_loc.second, MPI_UNSIGNED_CHAR, 
                (rank + size - 1) % size, 101, comm);
        MPI_Send(&cells[dimensions_loc.first][0], dimensions_loc.second, MPI_UNSIGNED_CHAR, 
                (rank + 1) % size, 102, comm);
        
        MPI_Wait(&req1, MPI_STATUS_IGNORE);
        MPI_Wait(&req2, MPI_STATUS_IGNORE);
    }
    
    // Public members
    std::pair<int, int> dimensions;
    std::pair<int, int> dimensions_loc;
    int start_loc;
    std::vector<std::vector<unsigned char>> cells;
    SDL_Color col_life;
    SDL_Color col_dead;
};

class App {
public:
    App(std::pair<int, int> geometry, Grille& grid) : grid(grid) {
        // Calculate cell size
        size_x = geometry.second / grid.dimensions.second;
        size_y = geometry.first / grid.dimensions.first;
        
        // Adjust window size to fit grid
        width = grid.dimensions.second * size_x;
        height = grid.dimensions.first * size_y;
        
        // Set draw color based on cell size
        if (size_x > 4 && size_y > 4) {
            draw_color = {192, 192, 192, 255}; // lightgrey
        } else {
            draw_color = {0, 0, 0, 0}; // transparent
        }
        
        // Initialize SDL
        SDL_Init(SDL_INIT_VIDEO);
        window = SDL_CreateWindow("Conway's Game of Life", 
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                                  width, height, SDL_WINDOW_SHOWN);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    }
    
    ~App() {
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    
    void draw() {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        
        // Draw cells
        for (int i = 1; i <= grid.dimensions_loc.first; ++i) {
            for (int j = 0; j < grid.dimensions_loc.second; ++j) {
                SDL_Rect rect = {
                    j * size_x, 
                    (i - 1 + grid.start_loc) * size_y, 
                    size_x, 
                    size_y
                };
                
                if (grid.cells[i][j] == 1) {
                    SDL_SetRenderDrawColor(renderer, grid.col_life.r, grid.col_life.g, grid.col_life.b, grid.col_life.a);
                } else {
                    SDL_SetRenderDrawColor(renderer, grid.col_dead.r, grid.col_dead.g, grid.col_dead.b, grid.col_dead.a);
                }
                
                SDL_RenderFillRect(renderer, &rect);
            }
        }
        
        // Draw grid lines
        if (draw_color.a != 0) {
            SDL_SetRenderDrawColor(renderer, draw_color.r, draw_color.g, draw_color.b, draw_color.a);
            
            // Horizontal lines
            for (int i = 0; i <= grid.dimensions.first; ++i) {
                SDL_RenderDrawLine(renderer, 0, i * size_y, width, i * size_y);
            }
            
            // Vertical lines
            for (int j = 0; j <= grid.dimensions.second; ++j) {
                SDL_RenderDrawLine(renderer, j * size_x, 0, j * size_x, height);
            }
        }
        
        SDL_RenderPresent(renderer);
    }
    
    Grille& grid;
    int size_x;
    int size_y;
    int width;
    int height;
    SDL_Color draw_color;
    SDL_Window* window;
    SDL_Renderer* renderer;
};

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    
    MPI_Comm globCom;
    MPI_Comm_dup(MPI_COMM_WORLD, &globCom);
    
    int rank, nbp;
    MPI_Comm_rank(globCom, &rank);
    MPI_Comm_size(globCom, &nbp);
    
    // Split communicator for worker processes
    int color = (rank != 0) ? 1 : 0;
    MPI_Comm newCom;
    MPI_Comm_split(globCom, color, rank, &newCom);
    
    int local_rank = 0, local_size = 0;
    MPI_Comm_rank(newCom, &local_rank);
    MPI_Comm_size(newCom, &local_size);
    
    std::cout << "rang global : " << rank << ", rang local : " << local_rank 
              << ", nb de processus locaux : " << local_size << std::endl;
    
    // Define patterns
    std::map<std::string, std::pair<std::pair<int, int>, std::vector<std::pair<int, int>>>> dico_patterns = {
        {"blinker", {std::make_pair(5, 5), {{2, 1}, {2, 2}, {2, 3}}}},
        {"toad", {std::make_pair(6, 6), {{2, 2}, {2, 3}, {2, 4}, {3, 3}, {3, 4}, {3, 5}}}},
        {"acorn", {std::make_pair(100, 100), {{51, 52}, {52, 54}, {53, 51}, {53, 52}, {53, 55}, {53, 56}, {53, 57}}}},
        {"beacon", {std::make_pair(6, 6), {{1, 3}, {1, 4}, {2, 3}, {2, 4}, {3, 1}, {3, 2}, {4, 1}, {4, 2}}}},
        {"boat", {std::make_pair(5, 5), {{1, 1}, {1, 2}, {2, 1}, {2, 3}, {3, 2}}}},
        {"glider", {std::make_pair(100, 90), {{1, 1}, {2, 2}, {2, 3}, {3, 1}, {3, 2}}}},
        {"glider_gun", {std::make_pair(200, 100), {{51, 76}, {52, 74}, {52, 76}, {53, 64}, {53, 65}, {53, 72}, {53, 73}, {53, 86}, {53, 87}, {54, 63}, {54, 67}, {54, 72}, {54, 73}, {54, 86}, {54, 87}, {55, 52}, {55, 53}, {55, 62}, {55, 68}, {55, 72}, {55, 73}, {56, 52}, {56, 53}, {56, 62}, {56, 66}, {56, 68}, {56, 69}, {56, 74}, {56, 76}, {57, 62}, {57, 68}, {57, 76}, {58, 63}, {58, 67}, {59, 64}, {59, 65}}}},
        {"space_ship", {std::make_pair(25, 25), {{11, 13}, {11, 14}, {12, 11}, {12, 12}, {12, 14}, {12, 15}, {13, 11}, {13, 12}, {13, 13}, {13, 14}, {14, 12}, {14, 13}}}},
        {"die_hard", {std::make_pair(100, 100), {{51, 57}, {52, 51}, {52, 52}, {53, 52}, {53, 56}, {53, 57}, {53, 58}}}},
        {"pulsar", {std::make_pair(17, 17), {
            {2, 4}, {2, 5}, {2, 6}, {7, 4}, {7, 5}, {7, 6}, {9, 4}, {9, 5}, {9, 6}, {14, 4}, {14, 5}, {14, 6},
            {2, 10}, {2, 11}, {2, 12}, {7, 10}, {7, 11}, {7, 12}, {9, 10}, {9, 11}, {9, 12}, {14, 10}, {14, 11}, {14, 12},
            {4, 2}, {5, 2}, {6, 2}, {4, 7}, {5, 7}, {6, 7}, {4, 9}, {5, 9}, {6, 9}, {4, 14}, {5, 14}, {6, 14},
            {10, 2}, {11, 2}, {12, 2}, {10, 7}, {11, 7}, {12, 7}, {10, 9}, {11, 9}, {12, 9}, {10, 14}, {11, 14}, {12, 14}
        }}},
        {"floraison", {std::make_pair(40, 40), {{19, 18}, {19, 19}, {19, 20}, {20, 17}, {20, 19}, {20, 21}, {21, 18}, {21, 19}, {21, 20}}}},
        {"block_switch_engine", {std::make_pair(400, 400), {{201, 202}, {201, 203}, {202, 202}, {202, 203}, {211, 203}, {212, 204}, {212, 202}, {214, 204}, {214, 201}, {215, 201}, {215, 202}, {216, 201}}}},
        {"u", {std::make_pair(200, 200), {{101, 101}, {102, 102}, {103, 102}, {103, 101}, {104, 103}, {105, 103}, {105, 102}, {105, 101}, {105, 105}, {103, 105}, {102, 105}, {101, 105}, {101, 104}}}},
        {"flat", {std::make_pair(200, 400), {{80, 200}, {81, 200}, {82, 200}, {83, 200}, {84, 200}, {85, 200}, {86, 200}, {87, 200}, {89, 200}, {90, 200}, {91, 200}, {92, 200}, {93, 200}, {97, 200}, {98, 200}, {99, 200}, {106, 200}, {107, 200}, {108, 200}, {109, 200}, {110, 200}, {111, 200}, {112, 200}, {114, 200}, {115, 200}, {116, 200}, {117, 200}, {118, 200}}}}
    };
    
    // Parse command line arguments
    std::string choice = "glider";
    if (argc > 1) {
        choice = argv[1];
    }
    
    int resx = 800;
    int resy = 800;
    if (argc > 3) {
        resx = std::stoi(argv[2]);
        resy = std::stoi(argv[3]);
    }
    
    if (rank == 0) {
        std::cout << "Pattern initial choisi : " << choice << std::endl;
        std::cout << "resolution ecran : " << resx << ", " << resy << std::endl;
    }
    
    // Check if pattern exists
    if (dico_patterns.find(choice) == dico_patterns.end()) {
        if (rank == 0) {
            std::cout << "No such pattern. Available ones are: ";
            for (const auto& p : dico_patterns) {
                std::cout << p.first << " ";
            }
            std::cout << std::endl;
        }
        MPI_Finalize();
        return 1;
    }
    
    auto pattern = dico_patterns[choice];
    
    if (rank == 0) {
        // Display process
        SDL_Init(SDL_INIT_VIDEO);
        Grille grid(0, 1, pattern.first, &pattern.second);
        App app(std::make_pair(resx, resy), grid);
        
        bool loop = true;
        while (loop) {
            int signal = 1;
            MPI_Send(&signal, 1, MPI_INT, 1, 0, globCom);
            
            std::vector<unsigned char> global_cells(pattern.first.first * pattern.first.second);
            MPI_Recv(global_cells.data(), global_cells.size(), MPI_UNSIGNED_CHAR, 1, 0, globCom, MPI_STATUS_IGNORE);
            
            // Convert received data to 2D array
            for (int i = 0; i < pattern.first.first; ++i) {
                for (int j = 0; j < pattern.first.second; ++j) {
                    grid.cells[i+1][j] = global_cells[i * pattern.first.second + j];
                }
            }
            
            auto t2 = std::chrono::high_resolution_clock::now();
            app.draw();
            auto t3 = std::chrono::high_resolution_clock::now();
            
            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    loop = false;
                    signal = -1;
                    MPI_Send(&signal, 1, MPI_INT, 1, 0, globCom);
                }
            }
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count() / 1000000.0;
            std::cout << "Temps affichage : " << duration << " secondes" << std::endl;
        }
        
        SDL_Quit();
    } else {
        // Worker process
        Grille grid(local_rank, local_size, pattern.first, &pattern.second);
        grid.update_ghost_cells(newCom);
        
        if (local_rank == 0) {
            std::cout << "rank loc : " << local_rank << ", cells locales : " << std::endl;
            for (int i = 0; i < grid.cells.size(); ++i) {
                for (int j = 0; j < grid.cells[0].size(); ++j) {
                    std::cout << (int)grid.cells[i][j] << " ";
                }
                std::cout << std::endl;
            }
        }
        
        // Gather the cell counts from all processes
        std::vector<int> sendcounts;
        if (local_rank == 0) {
            sendcounts.resize(local_size);
        }
        
        int local_cell_count = grid.cells.size() * grid.cells[0].size() - 2 * grid.cells[0].size(); // Exclude ghost cells
        MPI_Gather(&local_cell_count, 1, MPI_INT, sendcounts.data(), 1, MPI_INT, 0, newCom);
        
        // Create global grid buffer for rank 0 of newCom
        std::vector<unsigned char> grid_glob;
        if (local_rank == 0) {
            grid_glob.resize(pattern.first.first * pattern.first.second);
        }
        
        // Create flattened cell array (excluding ghost cells) for gathering
        std::vector<unsigned char> flat_cells;
        for (int i = 1; i <= grid.dimensions_loc.first; ++i) {
            for (int j = 0; j < grid.dimensions_loc.second; ++j) {
                flat_cells.push_back(grid.cells[i][j]);
            }
        }
        
        // Prepare displacement array for MPI_Gatherv
        std::vector<int> displs;
        if (local_rank == 0) {
            displs.resize(local_size);
            int disp = 0;
            for (int i = 0; i < local_size; ++i) {
                displs[i] = disp;
                disp += sendcounts[i];
            }
        }
        
        bool loop = true;
        while (loop) {
            // Optional sleep to limit frame rate
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            auto t1 = std::chrono::high_resolution_clock::now();
            grid.compute_next_iteration();
            grid.update_ghost_cells(newCom);
            auto t2 = std::chrono::high_resolution_clock::now();
            
            // Gather data from all processes to rank 0 of newCom
            MPI_Gatherv(flat_cells.data(), flat_cells.size(), MPI_UNSIGNED_CHAR,
                      grid_glob.data(), sendcounts.data(), displs.data(),
                      MPI_UNSIGNED_CHAR, 0, newCom);
            
            // Process 0 of newCom communicates with display process
            if (local_rank == 0) {
                int signal = 0;
                if (MPI_Iprobe(0, 0, globCom, &signal, MPI_STATUS_IGNORE) && signal) {
                    MPI_Recv(&signal, 1, MPI_INT, 0, 0, globCom, MPI_STATUS_IGNORE);
                    if (signal == -1) {
                        loop = false;
                    } else {
                        MPI_Send(grid_glob.data(), grid_glob.size(), MPI_UNSIGNED_CHAR, 0, 0, globCom);
                    }
                }
            }
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() / 1000000.0;
            std::cout << "Temps calcul prochaine generation : " << duration << " secondes" << std::endl;
        }
    }
    
    MPI_Finalize();
    return 0;
}