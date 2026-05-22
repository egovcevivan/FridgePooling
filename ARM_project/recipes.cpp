#include <iostream>
#include <fstream>
#include <map>
#include <cctype>

#include <QDebug>

#include "json.hpp"
#include "recipes.h"
#include "prods.h"


using json = nlohmann::json;

bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

bool is_exists(std::string path) {
    std::ifstream f(path);

    if (!f.is_open() || f.peek() == std::ifstream::traits_type::eof()) {
        std::cout << "Json don`t find" << std::endl;

        f.close();
        json j_file = json::object();
        std::ofstream out(path);
        out << std::setw(4) << j_file;
        out.close();

        return false;
    }
    else {
        std::cout << "Json find" << std::endl;
        return true;
    }
}

void add_recipe(std::string name, std::map<std::string, int> products) {
    std::ifstream in(RECIPES_PATH);
    json j_file = json::parse(in);
    in.close();

    j_file[name]["Ингредиенты"] = products;
    j_file[name]["Доступность"] = false;
    
    for (std::map<std::string, int>::const_iterator i = products.begin(); i != products.end(); i++) {
        std::string ingredient = i->first;
        int quantity = i->second;
        j_file[name]["Ингредиенты"][ingredient] = {{"Количество", quantity}, {"Наличие", false}};
    }

    std::ofstream out(RECIPES_PATH);
    out << std::setw(4) << j_file;
    out.close();
}

void add_recipe_ingr(std::string name, std::string ingr, int cnt) {
    if (is_exists(RECIPES_PATH)) {
        std::ifstream in(RECIPES_PATH);
        json j_file = json::parse(in);
        in.close();

        if (j_file.contains(name)) {
            if (!j_file[name]["Ингредиенты"].contains(ingr)) {
                j_file[name]["Ингредиенты"][ingr] = {{"Количество", cnt}, {"Наличие", false}};
                std::ofstream out(RECIPES_PATH);
                out << std::setw(4) << j_file;
                out.close();
            }
            else {
                std::cout << "Ingr is already taken" << std::endl;
            }
        }
        else {
            throw std::invalid_argument("add_recipe_ingr: Name don`t find");
        }
    }
}

void update_recipe_name(std::string name, std::string new_name) {
    if (is_exists(RECIPES_PATH)) {
        std::ifstream in(RECIPES_PATH);
        json j_file = json::parse(in);
        in.close();

        if (j_file.contains(name) && !j_file.contains(new_name)) {
            j_file[new_name] = j_file[name];
            j_file.erase(name);

            std::ofstream out(RECIPES_PATH);
            out << std::setw(4) << j_file;
            out.close();
        }
        else if (j_file.contains(new_name)) {
            throw std::invalid_argument("update_recipe_name: Name is already taken");
        }
    }
}

void update_recipe_ingr(std::string name, std::string ingr, std::string new_ingr) {
    if (is_exists(RECIPES_PATH)) {
        std::ifstream in(RECIPES_PATH);
        json j_file = json::parse(in);
        in.close();

        if (j_file.contains(name)) {
            if (j_file[name]["Ингредиенты"].contains(ingr) && !j_file[name]["Ингредиенты"].contains(new_ingr)) {
                j_file[name]["Ингредиенты"][new_ingr] = j_file[name]["Ингредиенты"][ingr];
                j_file[name]["Ингредиенты"].erase(ingr);

                std::ofstream out(RECIPES_PATH);
                out << std::setw(4) << j_file;
                out.close();
            }
            else if (j_file[name]["Ингредиенты"].contains(new_ingr)) {
                throw std::invalid_argument("update_recipe_ingr: Name is already taken");
            } 
        }
        else {
            std::cout << "Name dont find" << std::endl;
            return;
        }
    }
}

void update_recipe_ingr_cnt(std::string name, std::string ingr, int cnt) {
    if (is_exists(RECIPES_PATH)) {
        std::ifstream in(RECIPES_PATH);
        json j_file = json::parse(in);
        in.close();

        if (j_file.contains(name)) {
            if (j_file[name]["Ингредиенты"].contains(ingr) && j_file[name]["Ингредиенты"][ingr]["Количество"] != cnt) {
                j_file[name]["Ингредиенты"][ingr]["Количество"] = cnt;
                std::ofstream out(RECIPES_PATH);
                out << std::setw(4) << j_file;
                out.close();
            }
            else if (!j_file[name]["Ингредиенты"].contains(ingr)) {
                throw std::invalid_argument("update_recipe_ingr_cnt: Name ingr don`t find");
            } 
        }
        else {
            throw std::invalid_argument("update_recipe_ingr_cnt: Name don`t find");
        } 
    }
}



void del_recipe(std::string name) {
    if (is_exists(RECIPES_PATH)) {
        std::ifstream in(RECIPES_PATH);
        json j_file = json::parse(in);
        in.close();

        if (j_file.contains(name)) {
            j_file.erase(name);
            std::ofstream out(RECIPES_PATH);
            out << std::setw(4) << j_file;
            out.close();
        }
        else {
            throw std::invalid_argument("del_recipe: Name don`t find");
        }
    }
}

void del_recipe_ingr(std::string name, std::string ingr) {
    if (is_exists(RECIPES_PATH)) {
        std::ifstream in(RECIPES_PATH);
        json j_file = json::parse(in);
        in.close();

        if (j_file.contains(name)) {
            if (j_file[name]["Ингредиенты"].contains(ingr)) {
                j_file[name]["Ингредиенты"].erase(ingr);
                std::ofstream out(RECIPES_PATH);
                out << std::setw(4) << j_file;
                out.close();

                update_recipe_avail(name);
            }
            else {
                throw std::invalid_argument("del_recipe_ingr: Ingrenient don`t find");
            }
        }
        else {
            throw std::invalid_argument("del_recipe_ingr: Name don`t find");
        } 
    }
}

void update_product_in_recipes(const std::string& productName) {
    if (!is_exists(PRODS_PATH) || !is_exists(RECIPES_PATH)) {
        return;
    }

    std::ifstream f_prods(PRODS_PATH);
    json j_prods = json::parse(f_prods);
    f_prods.close();

    int total_volume = 0;
    for (auto& prod_item : j_prods.items()) {
        if (iequals(prod_item.value()["Название"].get<std::string>(), productName)) {
            total_volume += prod_item.value()["Общий объём"].get<int>();
        }
    }
    std::ifstream f_recipes(RECIPES_PATH);
    json j_recipes = json::parse(f_recipes);
    f_recipes.close();

    bool changed = false;

    for (auto& recipe_item : j_recipes.items()) {
        auto& ingredients = recipe_item.value()["Ингредиенты"];
        bool recipe_affected = false;

        for (auto& ingr_item : ingredients.items()) {
            const std::string& ingrName = ingr_item.key();
            if (iequals(ingrName, productName)) {
                recipe_affected = true;
                int needed = ingr_item.value()["Количество"].get<int>();
                bool new_flag = (total_volume >= needed);
                bool old_flag = ingr_item.value()["Наличие"].get<bool>();
                if (new_flag != old_flag) {
                    ingr_item.value()["Наличие"] = new_flag;
                    changed = true;
                }
            }
        }

        if (recipe_affected) {
            bool all_available = true;
            for (auto& ingr_item : ingredients.items()) {
                if (!ingr_item.value()["Наличие"].get<bool>()) {
                    all_available = false;
                    break;
                }
            }
            if (recipe_item.value()["Доступность"].get<bool>() != all_available) {
                recipe_item.value()["Доступность"] = all_available;
                changed = true;
            }
        }
    }

    if (changed) {
        std::ofstream out(RECIPES_PATH);
        out << std::setw(4) << j_recipes;
        out.close();
    }
}

void update_recipe_avail(std::string name) {
    if (is_exists(RECIPES_PATH)) {
        std::ifstream in(RECIPES_PATH);
        json j_file = json::parse(in);
        in.close();

        if (j_file.contains(name)) {
            for (json::iterator i = j_file[name]["Ингредиенты"].begin(); i != j_file[name]["Ингредиенты"].end(); i++) {
                if (j_file[name]["Ингредиенты"][i.key()]["Наличие"] == false) {
                    j_file[name]["Доступность"] = false;
                    std::ofstream out(RECIPES_PATH);
                    out << std::setw(4) << j_file;
                    out.close();
                    std::cout << "Not all products are available" << std::endl;
                    return;
                }
            }
            j_file[name]["Доступность"] = true;
            std::ofstream out(RECIPES_PATH);
            out << std::setw(4) << j_file;
            out.close();
        }
        else {
            throw std::invalid_argument("update_recipe_avail: Name don`t find");
        }
    }
}

void update_recipe_ingr_avail(std::string name, std::string ingr) {
    if (is_exists(RECIPES_PATH)) {
        std::ifstream in(RECIPES_PATH);
        json j_file = json::parse(in);
        in.close();

        if (j_file.contains(name)) {
            if (j_file[name]["Ингредиенты"].contains(ingr)) {
                j_file[name]["Ингредиенты"][ingr]["Наличие"] = !j_file[name]["Ингредиенты"][ingr]["Наличие"];
                std::ofstream out(RECIPES_PATH);
                out << std::setw(4) << j_file;
                out.close();

                update_recipe_avail(name);
            }
            else {
                throw std::invalid_argument("update_recipe_ingr_avail: Name ingr don`t find");
            }
        }
        else {
            throw std::invalid_argument("update_recipe_ingr_avail: Name don`t find");
        }
    }
}

void set_recipe_ingr_availability(const std::string& recipeName, const std::string& ingredient, bool available) {
    if (!is_exists(RECIPES_PATH)) return;
    std::ifstream in(RECIPES_PATH);
    json j_file = json::parse(in);
    in.close();

    if (!j_file.contains(recipeName)) {
        return;
    }

    if (!j_file[recipeName]["Ингредиенты"].contains(ingredient)) {
        return;
    }

    if (j_file[recipeName]["Ингредиенты"][ingredient]["Наличие"].get<bool>() != available) {
        j_file[recipeName]["Ингредиенты"][ingredient]["Наличие"] = available;
        std::ofstream out(RECIPES_PATH);
        out << std::setw(4) << j_file;
        out.close();
        update_recipe_avail(recipeName);
    }
}

void recalculate_recipe(const std::string& recipeName) {
    if (!is_exists(PRODS_PATH) || !is_exists(RECIPES_PATH)) return;

    std::ifstream f_prods(PRODS_PATH);
    json j_prods = json::parse(f_prods);
    f_prods.close();

    std::ifstream f_recipes(RECIPES_PATH);
    json j_recipes = json::parse(f_recipes);
    f_recipes.close();

    if (!j_recipes.contains(recipeName)) return;

    auto& recipe = j_recipes[recipeName];
    auto& ingredients = recipe["Ингредиенты"];
    bool all_available = true;

    for (auto& ingr_item : ingredients.items()) {
        const std::string& ingrName = ingr_item.key();
        int needed = ingr_item.value()["Количество"].get<int>();

        int total_volume = 0;
        for (auto& prod_item : j_prods.items()) {
            if (iequals(prod_item.value()["Название"].get<std::string>(), ingrName)) {
                total_volume += prod_item.value()["Общий объём"].get<int>();
            }
        }

        bool available = (total_volume >= needed);
        ingredients[ingrName]["Наличие"] = available;
        if (!available) all_available = false;
    }

    recipe["Доступность"] = all_available;

    std::ofstream out(RECIPES_PATH);
    out << std::setw(4) << j_recipes;
    out.close();
}
