#include <iostream>
#include <fstream>
#include <QInputDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QPushButton>
#include <QStringList>
#include <QDate>

#include "json.hpp"
#include "recipes.h"
#include "prods.h"
#include "web_parsing.h"

using json = nlohmann::json;

bool isRussian(const QString &text) {
    QRegularExpression re("^[А-Яа-яЁё\\s\\d\\-\\.%]+$");
    return re.match(text).hasMatch();
}

bool is_in(const std::string barcode) {
    if (!is_exists(PRODS_PATH)) {
        return false;
    }
    std::ifstream in(PRODS_PATH);

    if (!in.is_open()) {
        return false;
    }
    json j_file = json::parse(in);
    in.close();
    return j_file.contains(barcode);
}

void enter_prod(std::string barcode) {
    bool ok;
    QString name;
    while (true) {
        name = QInputDialog::getText(nullptr, "Ручной ввод",
            "Введите название продукта (на русском языке):",
            QLineEdit::Normal, "", &ok);

        if (!ok || name.isEmpty()) {
            return;
        }

        if (isRussian(name)) {
            break;
        }
        else {
            QMessageBox::warning(nullptr, "Ошибка", "Название должно содержать только русские буквы. Попробуйте снова.");
        }
    }
    QString brand = QInputDialog::getText(nullptr, "Ручной ввод", "Введите марку:", QLineEdit::Normal, "", &ok);
    if (!ok) {
        return;
    }
    int volume = QInputDialog::getInt(nullptr, "Ручной ввод", "Введите объём (мл или г или кол-во):", 100, 1, 10000, 1, &ok);

    if (!ok || volume <= 0) {
        return;
    }
    QString allergens = QInputDialog::getText(nullptr, "Ручной ввод", "Введите аллергены (через запятую):", QLineEdit::Normal, "Нет данных", &ok);

    if (!ok) {
        return;
    }

    if (check_product_allergens(allergens)) {
        QMessageBox::warning(nullptr, "Внимание", "Этот продукт содержит аллергены, которые вы указали!");
    }
    int expiry_days = QInputDialog::getInt(nullptr, "Срок годности", "Введите срок годности после вскрытия (в днях, 0 - не отслеживать):", 0, 0, 365, 1, &ok);

    if (!ok) {
        return;
    }

    std::ifstream in(PRODS_PATH);
    json j_file;

    if (in.is_open() && in.peek() != EOF) {
        j_file = json::parse(in);
    }
    else {
        j_file = json::object();
    }
    in.close();

    j_file[barcode]["Название"] = name.toStdString();
    j_file[barcode]["Марка"] = brand.toStdString();
    j_file[barcode]["Количество"] = 1;
    j_file[barcode]["Объём"] = volume;
    j_file[barcode]["Общий объём"] = volume;
    j_file[barcode]["Аллергены"] = allergens.toStdString();
    j_file[barcode]["Срок годности после вскрытия"] = expiry_days;

    std::ofstream out(PRODS_PATH);
    out << std::setw(4) << j_file;
    out.close();
    check_recipes_avail(name.toStdString());
    update_product_expiry_tracking(barcode);
}

void add_prod(std::string barcode) {
    Parser parser;
    int res = parser.web_request_sync(QString::fromStdString(barcode));

    if (res != 0) {
        enter_prod(barcode);
        return;
    }

    QString nameQ = parser.getProductName();
    QString brandQ = parser.getBrand();

    while (!isRussian(nameQ)) {
        bool ok;
        QString newName = QInputDialog::getText(nullptr, "Требуется русское название",
            "Название продукта должно быть на русском языке.\nВведите русское название:",
            QLineEdit::Normal, nameQ, &ok);
        if (!ok || newName.isEmpty()) {
            return;
        }

        if (isRussian(newName)) {
            nameQ = newName;
            break;
        }
        else {
            QMessageBox::warning(nullptr, "Ошибка", "Введённое название не содержит русских букв. Попробуйте снова.");
        }
    }

    std::string name = nameQ.toStdString();
    std::string mark = brandQ.toStdString();

    QString message = QString("Продукт найден:\nНазвание: %1\nБренд: %2\nВведите объём (мл или г):").arg(nameQ, brandQ);
    bool ok;

    int volume = QInputDialog::getInt(nullptr, "Добавление продукта", message, 100, 1, 10000, 1, &ok);
    if (!ok) {
        return;
    }
    int expiry_days = QInputDialog::getInt(nullptr, "Срок годности", "Введите срок годности после вскрытия (в днях, 0 - не отслеживать):", 0, 0, 365, 1, &ok);

    if (!ok) {
        return;
    }
    std::string allergens;
    QStringList allergList = parser.getAllergens();

    if (!allergList.isEmpty()) {
        allergens = allergList.join(", ").toStdString();
    }
    else {
        allergens = "Нет данных";
    }

    if (check_product_allergens(QString::fromStdString(allergens))) {
        QMessageBox::warning(nullptr, "Внимание", "Этот продукт содержит аллергены, которые вы указали!");
    }

    std::ifstream in(PRODS_PATH);
    json j_file;

    if (in.is_open() && in.peek() != std::ifstream::traits_type::eof()) {
        j_file = json::parse(in);
    }
    else {
        j_file = json::object();
    }
    in.close();

    j_file[barcode]["Название"] = name;
    j_file[barcode]["Марка"] = mark;
    j_file[barcode]["Количество"] = 1;
    j_file[barcode]["Объём"] = volume;
    j_file[barcode]["Общий объём"] = volume;
    j_file[barcode]["Аллергены"] = allergens;
    j_file[barcode]["Срок годности после вскрытия"] = expiry_days;

    std::ofstream out(PRODS_PATH);
    out << std::setw(4) << j_file;
    out.close();
    check_recipes_avail(name);
    update_product_expiry_tracking(barcode);
}

void increase_prod(std::string barcode) {
    if (!is_exists(PRODS_PATH)) {
        return;
    }
    std::ifstream in(PRODS_PATH);
    json j_file = json::parse(in);
    in.close();

    if (!j_file.contains(barcode)) {
        add_prod(barcode); return;
    }
    int cnt = j_file[barcode]["Количество"].get<int>();
    int volume = j_file[barcode]["Объём"].get<int>();
    int total_volume = j_file[barcode]["Общий объём"].get<int>();
    std::string name = j_file[barcode]["Название"].get<std::string>();
    cnt++;
    j_file[barcode]["Общий объём"] = volume + total_volume;
    j_file[barcode]["Количество"] = cnt;

    std::ofstream out(PRODS_PATH);
    out << std::setw(4) << j_file;
    out.close();

    check_recipes_avail(name);
    update_product_expiry_tracking(barcode);
}

void reduce_prod(std::string barcode, int volume) {
    if (!is_exists(PRODS_PATH)) {
        return;
    }
    std::ifstream in(PRODS_PATH);
    json j_file = json::parse(in);
    in.close();

    if (!j_file.contains(barcode)) {
        throw std::invalid_argument("reduce_prod: Name don`t find");
    }
    int temp_volume = j_file[barcode]["Объём"].get<int>();
    int summ_volume = j_file[barcode]["Общий объём"].get<int>();
    if (summ_volume < volume) return;
    int cnt = volume / temp_volume;
    int temp_cnt = j_file[barcode]["Количество"].get<int>() - cnt;
    summ_volume -= volume;

    if (summ_volume == 0) {
        temp_cnt = 0;
    }
    j_file[barcode]["Количество"] = temp_cnt;
    j_file[barcode]["Общий объём"] = summ_volume;
    std::ofstream out(PRODS_PATH);

    out << std::setw(4) << j_file;
    out.close();
    check_recipes_avail(j_file[barcode]["Название"].get<std::string>());
    update_product_expiry_tracking(barcode);
}

void del_prod(std::string barcode) {
    if (!is_exists(PRODS_PATH)) {
        return;
     }
    std::ifstream in(PRODS_PATH);
    json j = json::parse(in);
    in.close();

    if (!j.contains(barcode)) {
        return;
    }
    j[barcode]["Количество"] = 0;
    j[barcode]["Общий объём"] = 0;
    std::ofstream out(PRODS_PATH);

    out << std::setw(4) << j;
    out.close();
    std::string name = j[barcode]["Название"].get<std::string>();
    check_recipes_avail(name);
    update_product_expiry_tracking(barcode);
}

int get_product_count(const std::string& barcode) {
    if (!is_exists(PRODS_PATH)) {
        return 0;
    }
    std::ifstream in(PRODS_PATH);
    json j = json::parse(in);
    in.close();

    if (!j.contains(barcode)) {
        return 0;
    }

    return j[barcode]["Количество"].get<int>();
}

int get_product_volume_per_unit(const std::string barcode) {
    if (!is_exists(PRODS_PATH)) {
        return 0;
    }
    std::ifstream in(PRODS_PATH);
    json j = json::parse(in);
    in.close();

    if (!j.contains(barcode)) {
        return 0;
    }

    return j[barcode]["Объём"].get<int>();
}

int get_total_volume(const std::string barcode) {
    if (!is_exists(PRODS_PATH)) {
        return 0;
    }
    std::ifstream in(PRODS_PATH);
    json j = json::parse(in);
    in.close();

    if (!j.contains(barcode)) {
        return 0;
    }

    return j[barcode]["Общий объём"].get<int>();
}

void check_recipes_avail(std::string prod) {
    update_product_in_recipes(prod);
}

QStringList load_allergens() {
    std::ifstream in(ALLERGENS_PATH);

    if (!in.is_open()) {
        return QStringList();
    }

    try {
        json j = json::parse(in);
        in.close();
        if (!j.is_array()) {
            return QStringList();
        }
        QStringList list;

        for (const auto &item : j)
            if (item.is_string()) {
                list << QString::fromStdString(item.get<std::string>());
            }
        return list;
    } catch (...) {
        return QStringList();
    }
}

void save_allergens(const QStringList &allergens) {
    json j = json::array();

    for (const QString &a : allergens) {
        j.push_back(a.toStdString());
    }
    std::ofstream out(ALLERGENS_PATH);
    out << std::setw(4) << j;
    out.close();
}

void add_allergen(const QString &allergen) {
    if (allergen.trimmed().isEmpty()) {
        return;
    }
    QStringList list = load_allergens();

    if (!list.contains(allergen, Qt::CaseInsensitive)) {
        list << allergen;
        save_allergens(list);
    }
}

void remove_allergen(const QString &allergen) {
    QStringList list = load_allergens();

    if (list.removeAll(allergen)) {
        save_allergens(list);
    }
}

bool check_product_allergens(const QString &productAllergensStr) {
    const QStringList userAllergens = load_allergens();

    if (userAllergens.isEmpty()) {
        return false;
    }

    const QStringList productAllergens = productAllergensStr.split(QRegularExpression("[\\s,]+"), QString::SkipEmptyParts);

    for (const QString &user : qAsConst(userAllergens)) {
        for (const QString &prod : qAsConst(productAllergens)) {
            if (prod.contains(user, Qt::CaseInsensitive)) {
                return true;
            }
        }
    }
    return false;
}

void show_allergens_dialog(QWidget *parent) {
    QDialog dialog(parent);
    dialog.setWindowTitle("Мои аллергены");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QListWidget *listWidget = new QListWidget(&dialog);
    listWidget->addItems(load_allergens());
    layout->addWidget(listWidget);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Добавить");
    QPushButton *removeBtn = new QPushButton("Удалить");
    QPushButton *closeBtn = new QPushButton("Закрыть");
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    QObject::connect(addBtn, &QPushButton::clicked, &dialog, [&dialog, listWidget]() {
        bool ok = false;
        QString newAllergen = QInputDialog::getText(&dialog, "Новый аллерген", "Введите аллерген:", QLineEdit::Normal, "", &ok);

        if (ok && !newAllergen.isEmpty()) {
            add_allergen(newAllergen);
            listWidget->clear();
            listWidget->addItems(load_allergens());
        }
    });

    QObject::connect(removeBtn, &QPushButton::clicked, &dialog, [listWidget]() {
        QListWidgetItem *item = listWidget->currentItem();
        if (item) {
            remove_allergen(item->text());
            delete item;
        }
    });

    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

bool is_product_name_exists(const std::string& productName) {
    if (!is_exists(PRODS_PATH)) {
        return false;
    }
    std::ifstream in(PRODS_PATH);
    json j = json::parse(in);
    in.close();

    for (auto &item : j.items()) {
        if (item.value()["Название"] == productName) return true;
    }
    return false;
}

int get_days_since_opened(const json& j_prod) {
    if (!j_prod.contains("Отслеживать") || !j_prod["Отслеживать"].get<bool>()) {
        return -1;
    }

    if (!j_prod.contains("Дата вскрытия")) {
        return -1;
    }
    QString dateStr = QString::fromStdString(j_prod["Дата вскрытия"].get<std::string>());
    QDate openedDate = QDate::fromString(dateStr, Qt::ISODate);

    if (!openedDate.isValid()) {
        return -1;
    }
    return openedDate.daysTo(QDate::currentDate());
}

void update_product_expiry_tracking(const std::string& barcode) {
    if (!is_exists(PRODS_PATH)) {
        return;
    }
    std::ifstream in(PRODS_PATH);
    json j = json::parse(in);
    in.close();

    if (!j.contains(barcode)) {
        return;
    }
    auto& prod = j[barcode];
    int total_volume = prod["Общий объём"].get<int>();
    int unit_volume = prod["Объём"].get<int>();
    bool currently_tracking = prod.value("Отслеживать", false);
    bool should_track = (total_volume % unit_volume != 0);

    if (should_track && !currently_tracking) {
        prod["Отслеживать"] = true;
        prod["Дата вскрытия"] = QDate::currentDate().toString(Qt::ISODate).toStdString();
    }
    else if (!should_track && currently_tracking) {
        prod.erase("Отслеживать");
        prod.erase("Дата вскрытия");
    }

    std::ofstream out(PRODS_PATH);
    out << std::setw(4) << j;
    out.close();
}

bool is_product_expired(const std::string& barcode) {
    if (!is_exists(PRODS_PATH)) {
        return false;
    }
    std::ifstream in(PRODS_PATH);
    json j = json::parse(in);
    in.close();

    if (!j.contains(barcode)) {
        return false;
    }
    auto& prod = j[barcode];

    if (!prod.value("Отслеживать", false)) {
        return false;
    }
    int shelf_life = prod.value("Срок годности после вскрытия", 0);

    if (shelf_life <= 0) {
        return false;
    }
    int days = get_days_since_opened(prod);

    if (days < 0) {
        return false;
    }
    return days >= shelf_life;
}

QStringList get_expired_products() {
    QStringList expired;

    if (!is_exists(PRODS_PATH)) {
        return expired;
    }
    std::ifstream in(PRODS_PATH);
    json j = json::parse(in);
    in.close();

    for (auto& item : j.items()) {
        const std::string& barcode = item.key();
        if (is_product_expired(barcode)) {
            std::string name = item.value()["Название"].get<std::string>();
            expired << QString::fromStdString(name);
        }
    }
    return expired;
}

void discard_expired_product(const std::string& barcode) {
    if (!is_exists(PRODS_PATH)) {
        return;
    }
    std::ifstream in(PRODS_PATH);
    json j = json::parse(in);
    in.close();

    if (!j.contains(barcode)) {
        return;
    }
    auto& prod = j[barcode];
    int unit_volume = prod["Объём"].get<int>();
    int total_volume = prod["Общий объём"].get<int>();
    int remainder = total_volume % unit_volume;

    if (remainder != 0) {
        int new_total_volume = total_volume - remainder;
        int new_count = new_total_volume / unit_volume;
        prod["Общий объём"] = new_total_volume;
        prod["Количество"] = new_count;
    }
    std::ofstream out(PRODS_PATH);
    out << std::setw(4) << j;
    out.close();

    update_product_expiry_tracking(barcode);
    std::string name = prod["Название"].get<std::string>();
    check_recipes_avail(name);
}
