#ifndef RECIPES
#define RECIPES

#include <map>

#include "json.hpp"

#define RECIPES_PATH "recipes.json"


using json = nlohmann::json;

bool is_exists(std::string path);
void add_recipe(std::string name, std::map<std::string, int> products);
void add_recipe_ingr(std::string name, std::string ingr, int cnt);
void update_recipe_name(std::string name, std::string new_name);
void update_recipe_ingr(std::string name, std::string ingr, std::string new_ingr);
void update_recipe_ingr_cnt(std::string name, std::string ingr, int cnt);
void update_recipe_avail(std::string name);
void update_product_in_recipes(const std::string& productName);
void recalculate_recipe(const std::string& recipeName);
void del_recipe(std::string name);
void del_recipe_ingr(std::string name, std::string ingr);
bool iequals(const std::string& a, const std::string& b);

#endif
