#ifndef PRODUCTS
#define PRODUCTS

#include <string>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "json.hpp"

#define PRODS_PATH "prods.json"
#define ALLERGENS_PATH "allergens.json"


bool is_in(std::string barcode);
void enter_prod(std::string barcode);
void add_prod(std::string barcode);
void increase_prod(std::string barcode);
void reduce_prod(std::string barcode, int volume);
void del_prod(std::string barcode);
void check_recipes_avail(std::string prod);
int get_product_count(const std::string& barcode);
int get_product_volume_per_unit(const std::string barcode);
int get_total_volume(const std::string barcode);
bool is_product_name_exists(const std::string& productName);
void update_product_expiry_tracking(const std::string& barcode);
bool is_product_expired(const std::string& barcode);
void discard_expired_product(const std::string& barcode);
void save_allergens(const QStringList &allergens);
void add_allergen(const QString &allergen);
void remove_allergen(const QString &allergen);
bool check_product_allergens(const QString &productAllergensStr);
void show_allergens_dialog(QWidget *parent = nullptr);
QStringList get_expired_products();
QStringList load_allergens();

#endif
