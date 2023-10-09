#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <cmath>
#include <algorithm>
#include <chrono>

char f_chosen = 1;
// define target function
// original range 0-20000000, mapping to -10.0000,10.0000
double f1(unsigned int rx, unsigned int ry)
{
    double x = rx / 1000000.0 - 10.0;
    double y = ry / 1000000.0 - 10.0;
    return std::sin(x)/x * std::sin(y)/y;
}

// define target function
// original range 0-20000000, mapping to 0.000000,10.000000
double f3(unsigned int rx, unsigned int ry)
{
    double x = rx / 2000000.0;
    double y = ry / 2000000.0;
    return 6.452*(x + 0.125*y)*(std::cos(x) - std::cos(2*y))*(std::cos(x) - std::cos(2*y)) / std::sqrt(0.8 + (x - 4.2)*(x - 4.2) + 2*(y-7)*(y-7))  + 3.226*y;
}

// select target function
double f(unsigned int rx, unsigned int ry)
{
    if(f_chosen == 1)
    {
        return f1(rx,ry);
    }
    else if (3 == f_chosen)
    {
        return f3(rx,ry);
    }

}
// define individual class
struct Individual
{
    int rx;
    int ry;
    double fitness;

    Individual(int rx, int ry) : rx(rx), ry(ry), fitness(f(rx, ry)) {}
};

// define population class
class Population
{
    private:
    int size;
    char chromosome_length = 24;
    float pc = 1.0;
    float pm = 1.0;
    int upper_limit = 20000000;
    int lower_limit = 0;
    std::vector<Individual> individuals;
    unsigned rd_seed = std::chrono::system_clock::now().time_since_epoch().count();
    public:
    Population(int size,float pc,float pm) : size(size),pc(pc),pm(pm)
    {
        std::random_device rd;
        // random seed
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 generator(seed);
        std::uniform_int_distribution<int> distribution(lower_limit, upper_limit);

        for (int i = 0; i < size; ++i)
        {
            int rx = distribution(generator);
            int ry = distribution(generator);
            individuals.emplace_back(rx, ry);
        }
    }
    // selection operation
    std::vector<Individual> selection()
    {

        std::sort(individuals.begin(), individuals.end(),
                  [](const Individual& ind1, const Individual& ind2)
        {
            return ind1.fitness > ind2.fitness;
        });

        individuals = std::vector<Individual>(individuals.begin(), individuals.begin() + size);


        std::vector<Individual> selected;
        double totalFitness = 0.0;

        // calculate total fitness
        for (const auto& ind : individuals)
        {
            totalFitness += ind.fitness;
        }

        // calculate chosen probability for each individual
        std::vector<double> probabilities;
        for (const auto& ind : individuals)
        {
            double probability = ind.fitness / totalFitness;
            probabilities.push_back(probability);
        }

        // select due to probability
        std::random_device rd;
        std::mt19937 generator((unsigned) time(0));
        std::uniform_real_distribution<double> distribution(0.0, 1.0);

        while (selected.size() < size / 2)
        {
            double randomNum = distribution(generator);
            double cumulativeProbability = 0.0;

            for (int i = 0; i < individuals.size(); ++i)
            {
                cumulativeProbability += probabilities[i];
                if (randomNum <= cumulativeProbability)
                {
                    selected.push_back(individuals[i]);
                    break;
                }
            }
        }

        return selected;
    }


    // crossover operation
    std::vector<Individual> crossover(const std::vector<Individual>& parents)
    {
        std::vector<Individual> offspring;
        std::random_device rd;
        std::mt19937 generator((unsigned) time(0));
        std::uniform_int_distribution<int> distribution(0, chromosome_length-1); //int has a total of 32 bits, with 24 bits being useful. Choose one bit to perform an operation on.
        int MAX_ATTEMPTS = 20;

        for (int i = 0; i < parents.size(); ++i)
        {
            const Individual& parent1 = parents[i];
            const Individual& parent2 = parents[rd() % parents.size()]; // Randomly select parent2

            int x1 = parent1.rx;
            int x2 = parent2.rx;
            bool fcrossover = false;

            for (int j = 0; j < MAX_ATTEMPTS; ++j)
            {
                int bit = distribution(generator);
                //std::cout<<"rx bit is "<<bit<<std::endl;

                int mask = (parent2.rx & (~0 << bit)); // Set mask to include all bits after the given bit position
                //std::cout<<"mask is "<<mask<<std::endl;

                if ((parent1.rx & mask) != (parent2.rx & mask))
                {
                    x1 = (parent1.rx & ~mask) | (parent2.rx & mask);
                    //std::cout<<"x1 is "<<x1<<std::endl;

                    if(x1 > lower_limit && x1 < upper_limit)
                    {
                        fcrossover = true;
                        //std::cout<<"fcrossover"<<std::endl;
                        break;
                    }
                }
            }

            int y1 = parent1.ry;
            int y2 = parent2.ry;

            for (int j = 0; j < MAX_ATTEMPTS; ++j)
            {
                int bit = distribution(generator);
//                std::cout<<"ry bit is "<<bit<<std::endl;
                int mask = (parent2.ry & (~0 << bit)); // Set mask to include all bits after the given bit position
                if ((parent1.ry & mask) != (parent2.ry & mask))
                {
                    y1 = (parent1.ry & ~mask) | (parent2.ry & mask);
//                    std::cout<<"y1 is "<<y1<<std::endl;
                    if(y1 > lower_limit && y1 < upper_limit)
                    {
                        fcrossover = true;
//                        std::cout<<"fcrossover"<<std::endl;
                        break;
                    }
                }
            }

            if(!fcrossover) //remain the same
            {
                x1 = parent1.rx;
                y1 = parent1.ry;
            }
            offspring.emplace_back(x1, y1);
        }
        return offspring;
    }



    // mutation operation
    std::vector<Individual> mutation(std::vector<Individual>& offspring)
    {
        std::random_device rd;
        std::mt19937 generator((unsigned) time(0));
        std::uniform_int_distribution<int> bit_distribution(0, chromosome_length-1); //int has a total of 32 bits, with 24 bits being useful. Choose one bit to perform an operation on.
        std::uniform_int_distribution<int> value_distribution(0, upper_limit);

        int MAX_ATTEMPTS = 50;

        for (auto& ind : offspring)
        {
            bool fmutate = false;
            int rx = ind.rx;
            int ry = ind.ry;
            for (int j = 0; j < MAX_ATTEMPTS; ++j)
            {
                int bit = bit_distribution(generator);// Randomly select a bit
//                std::cout<<"rx bit is "<<bit<<std::endl;
                int mask = 1 << bit;
                rx = ind.rx ^ mask; // invert
                if(rx > lower_limit && rx < upper_limit)
                {
                    fmutate = true;
//                        std::cout<<"fmutate"<<std::endl;
                    break;
                }
            }

            fmutate = false;
            for (int j = lower_limit; j < MAX_ATTEMPTS; ++j)
            {
                int bit = bit_distribution(generator);// Randomly select a bit
//                std::cout<<"ry bit is "<<bit<<std::endl;
                int mask = 1 << bit;
                ry = ind.ry ^ mask; // invert
                if(ry > 0 && ry < upper_limit)
                {
                    fmutate = true;
//                        std::cout<<"fmutate"<<std::endl;
                    break;
                }
            }


            if(fmutate) //remain  the same
            {
                rx = ind.rx;
                ry = ind.ry;
            }
            else
            {
                ind.rx = rx;
                ind.ry = ry;

            }
            ind.fitness = f(rx, ry);

        }
        return offspring;
    }


    // evolve step
    void evolve()
    {
        std::random_device rd;
        std::mt19937 generator((unsigned) time(0));
        std::uniform_real_distribution<double> distribution(0.0, 1.0);

        std::vector<Individual> parents = selection();
        //std::cout<<"parents size "<<parents.size()<<std::endl;
        std::vector<Individual> offspring;

        double this_pc = distribution(generator);
        double this_pm = distribution(generator);

        if (this_pc < pc)
        {
            offspring = crossover(parents);
        }
        else
        {
            offspring = parents;
        }

        if (this_pm < pm)
        {
            offspring = mutation(offspring);
        }
        else
        {
            offspring = parents;
        }

        individuals = parents;
        individuals.insert(individuals.end(), offspring.begin(), offspring.end());
        //std::cout<<"individuals size "<<individuals.size()<<std::endl;
    }


    // get best indivdual
    Individual get_best_individual()
    {
        auto best = std::max_element(individuals.begin(), individuals.end(),
                                     [](const Individual& ind1, const Individual& ind2)
        {
            return ind1.fitness < ind2.fitness;
        });
        return *best;
    }
};

int main()
{
    // initialize pop
    int pop_size = 50;
    int iter_num = 500;
    float pc = 0.8, pm = 0.8;
    Population population(pop_size,pc,pm);
    double x,y;
    std::cout.precision(4); //set cout precision
    // open file to log
    std::ofstream file("data.txt");

    // main loop
    for (int i = 0; i < iter_num; ++i)
    {
        population.evolve();

        // get the best individual
        Individual best_individual = population.get_best_individual();
        if(f_chosen == 1)
        {
            x = best_individual.rx / 1000000.0 - 10.0;
            y = best_individual.ry / 1000000.0 - 10.0;
            std::cout.precision(4);
        }
        else if(f_chosen == 3)
        {
            x = best_individual.rx / 2000000.0;
            y = best_individual.ry / 2000000.0;
            std::cout.precision(7);
        }


        // write into file
        file << x << " " << y << " " << best_individual.fitness << std::endl;
        std::cout << x << " " << y << " " << best_individual.fitness << std::endl;
    }

    // close file
    file.close();

    return 0;
}
