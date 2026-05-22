#include <fstream>
#include <vector>
#include <algorithm>
#include <set>
#include <QSet>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QDate>
#include <QSpinBox>

#include "window.h"
#include "ui_window.h"
#include "prods.h"
#include "recipes.h"


Window::Window(QWidget *parent) : QMainWindow(parent), ui(new Ui::Window), m_skipNextClear(false) {
    ui->setupUi(this);
    ui->RecipesList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->AllergensList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->SelfLifeList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_successTimer = new QTimer(this);
    m_successTimer->setSingleShot(true);
    m_allergenButton = ui->AllergenButton;
    m_recipesManageButton = ui->RecipesButton;

    if (m_allergenButton) {
        m_allergenButton->setVisible(true);
        connect(m_allergenButton, &QPushButton::clicked, this, &Window::onAllergenButtonClicked);
    }

    if (m_recipesManageButton) {
        m_recipesManageButton->setVisible(true);
        connect(m_recipesManageButton, &QPushButton::clicked, this, &Window::onRecipesManageClicked);
    }
    connect(ui->LineEdit, &FixedLineEdit::validationResult, this, &Window::onValidationResult);
    connect(ui->LineEdit, &QLineEdit::textChanged, this, &Window::onTextChanged);
    connect(ui->RecipesList, &QListWidget::itemClicked, this, &Window::onRecipeClicked);
    connect(ui->AllergensList, &QListWidget::itemClicked, this, &Window::onAllergenItemClicked);
    connect(ui->SelfLifeList, &QListWidget::itemClicked, this, &Window::onSelfLifeItemClicked);
    connect(m_successTimer, &QTimer::timeout, this, [this]() {ui->EnterLabel->clear();});

    updateRecipesList();
    updateAllergensList();
    updateSelfLifeList();
}

Window::~Window() {
    delete ui;
}

void Window::onValidationResult(bool success, const QString &barcode) {
    if (!success) {
        m_successTimer->stop();
        ui->EnterLabel->setText(QString("Ошибка: нужно 13 цифр, введено %1").arg(barcode.length()));
        ui->EnterLabel->setStyleSheet("color: red;");
        return;
    }
    m_successTimer->start(3000);
    ui->EnterLabel->setText("Штрих-код принят!");
    ui->EnterLabel->setStyleSheet("color: green;");

    std::string barcodeStr = barcode.toStdString();
    bool productExisted = is_in(barcodeStr);

    if (!productExisted) {
        add_prod(barcodeStr);

        if (!is_in(barcodeStr)) {
            ui->EnterLabel->setText("Добавление отменено");
            ui->EnterLabel->setStyleSheet("color: orange;");
            m_successTimer->start(2000);
            m_skipNextClear = true;
            ui->LineEdit->clear();

            updateRecipesList();
            updateAllergensList();
            updateSelfLifeList();

            return;
        }
        bool ok;
        int count = QInputDialog::getInt(nullptr, "Добавление количества",
                                         QString("Сколько единиц продукта добавить?").arg(barcode),
                                         1, 1, 100, 1, &ok);
        if (ok && count > 0) {
            for (int i = 1; i < count; ++i) {
                increase_prod(barcodeStr);
            }
            ui->EnterLabel->setText(QString("Добавлено %1 единиц(ы)").arg(count));
            ui->EnterLabel->setStyleSheet("color: blue;");
            m_successTimer->start(2000);
        }
        else {
            ui->EnterLabel->setText("Добавление отменено");
            ui->EnterLabel->setStyleSheet("color: orange;");
            m_successTimer->start(2000);
        }
        updateRecipesList();
        updateAllergensList();
        updateSelfLifeList();

        m_skipNextClear = true;
        ui->LineEdit->clear();

        return;
    }
    QStringList actions;
    actions << "Добавить продукт" << "Уменьшить объём" << "Изменить параметры";
    bool ok;
    QString chosen = QInputDialog::getItem(nullptr, "Выберите действие", "Что сделать с продуктом?", actions, 0, false, &ok);

    if (!ok || chosen.isEmpty()) {
        m_skipNextClear = true;
        ui->LineEdit->clear();
        return;
    }

    if (chosen == "Добавить продукт") {
        int count = QInputDialog::getInt(nullptr, "Добавление количества",
                                         QString("Сколько единиц продукта добавить?").arg(barcode),
                                         1, 1, 100, 1, &ok);
        if (!ok || count <= 0) {
            ui->EnterLabel->setText("Добавление отменено");
            ui->EnterLabel->setStyleSheet("color: orange;");
            m_successTimer->start(2000);
        }
        else {
            for (int i = 0; i < count; ++i) {
                increase_prod(barcodeStr);
            }
            ui->EnterLabel->setText(QString("Добавлено %1 единиц(ы)").arg(count));
            ui->EnterLabel->setStyleSheet("color: blue;");
            m_successTimer->start(2000);
        }
    }
    else if (chosen == "Уменьшить объём") {
        int totalVolume = get_total_volume(barcodeStr);

        if (totalVolume <= 0) {
            QMessageBox::warning(nullptr, "Ошибка", "Общий объём продукта равен 0. Нечего уменьшать.");
            ui->EnterLabel->setText("Уменьшение невозможно");
            ui->EnterLabel->setStyleSheet("color: red;");
            m_successTimer->start(2000);
            m_skipNextClear = true;
            ui->LineEdit->clear();
            return;
        }
        int unitVolume = get_product_volume_per_unit(barcodeStr);
        int volumeToRemove = QInputDialog::getInt(nullptr, "Уменьшение объёма",
                                                  QString("Текущий общий объём: %1 мл/г/шт\nОбъём одной единицы: %2\nВведите объём для удаления:")
                                                  .arg(totalVolume).arg(unitVolume),
                                                  1, 1, totalVolume, 1, &ok);
        if (!ok || volumeToRemove <= 0) {
            ui->EnterLabel->setText("Уменьшение отменено");
            ui->EnterLabel->setStyleSheet("color: orange;");
            m_successTimer->start(2000);
        }
        else {
            reduce_prod(barcodeStr, volumeToRemove);
            ui->EnterLabel->setText(QString("Удалено %1 мл/г/шт").arg(volumeToRemove));
            ui->EnterLabel->setStyleSheet("color: orange;");
            m_successTimer->start(2000);
        }
    }
    else if (chosen == "Изменить параметры") {
            editProductParameters(barcodeStr);
    }
    m_skipNextClear = true;
    ui->LineEdit->clear();

    updateRecipesList();
    updateAllergensList();
    updateSelfLifeList();
}

void Window::editProductParameters(const std::string &barcode) {
    std::ifstream in(PRODS_PATH);

    if (!in.is_open()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл продуктов");
        return;
    }
    json j_prods = json::parse(in);
    in.close();

    if (!j_prods.contains(barcode)) {
        QMessageBox::warning(this, "Ошибка", "Продукт не найден");
        return;
    }
    auto &product = j_prods[barcode];
    std::string oldName = product["Название"].get<std::string>();
    int oldUnitVolume = product["Объём"].get<int>();
    int totalVolume = product["Общий объём"].get<int>();

    QDialog dialog(this);
    dialog.setWindowTitle("Изменение параметров продукта");
    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *nameLabel = new QLabel("Название продукта:");
    QLineEdit *nameEdit = new QLineEdit(QString::fromStdString(oldName));
    layout->addWidget(nameLabel);
    layout->addWidget(nameEdit);

    QLabel *volumeLabel = new QLabel("Объём одной единицы (мл/г/шт):");
    QSpinBox *volumeSpin = new QSpinBox;
    volumeSpin->setRange(1, 10000);
    volumeSpin->setValue(oldUnitVolume);
    layout->addWidget(volumeLabel);
    layout->addWidget(volumeSpin);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton("Сохранить");
    QPushButton *cancelBtn = new QPushButton("Отмена");
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }
    QString newNameQ = nameEdit->text().trimmed();

    if (newNameQ.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Название не может быть пустым");
        return;
    }
    std::string newName = newNameQ.toStdString();
    int newUnitVolume = volumeSpin->value();

    if (newName == oldName && newUnitVolume == oldUnitVolume) {
        return;
    }

    if (newName != oldName) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Изменение названия",
            QString("Вы меняете название продукта с \"%1\" на \"%2\".\n"
                    "Это может сделать некоторые рецепты недоступными (ингредиенты привязаны к названию).\n"
                    "Продолжить?").arg(QString::fromStdString(oldName), newNameQ),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) {
            return;
        }
    }

    if (newUnitVolume != oldUnitVolume) {
        int newCount = totalVolume / newUnitVolume;
        int newTotalVolume = newCount * newUnitVolume;
        int lostVolume = totalVolume - newTotalVolume;

        if (lostVolume > 0) {
            QMessageBox::warning(this, "Потеря объёма",
                QString("При изменении объёма единицы с %1 на %2, общий объём (%3 мл/г/шт) не делится нацело.\n"
                        "Будет сохранено %4 целых единиц, общий объём станет %5 мл/г/шт.\n"
                        "Потеряно %6 мл/г.").arg(oldUnitVolume).arg(newUnitVolume).arg(totalVolume)
                        .arg(newCount).arg(newTotalVolume).arg(lostVolume));
        }
        product["Количество"] = newCount;
        product["Общий объём"] = newTotalVolume;
        product["Объём"] = newUnitVolume;
    }

    if (newName != oldName) {
        product["Название"] = newName;
    }

    std::ofstream out(PRODS_PATH);
    out << std::setw(4) << j_prods;
    out.close();

    check_recipes_avail(oldName);

    if (newName != oldName) {
        check_recipes_avail(newName);
    }
    updateRecipesList();
    updateAllergensList();
    updateSelfLifeList();

    ui->EnterLabel->setText("Параметры продукта обновлены");
    ui->EnterLabel->setStyleSheet("color: blue;");
    m_successTimer->start(3000);
}

void Window::onTextChanged(const QString &text) {
    if (m_skipNextClear) {
        m_skipNextClear = false;
        return;
    }

    if (text.isEmpty()) {
        m_successTimer->stop();
        ui->EnterLabel->clear();
    }
}

void Window::onAllergenButtonClicked() {
    show_allergens_dialog(this);
    updateAllergensList();
}

void Window::updateRecipesList() {
    ui->RecipesList->clear();

    if (!is_exists(RECIPES_PATH)) {
        return;
    }
    std::ifstream in(RECIPES_PATH);

    if (!in.is_open()) {
        return;
    }
    json j_recipes = json::parse(in);
    in.close();

    for (auto &item : j_recipes.items()) {
        const std::string &recipeName = item.key();
        bool available = item.value()["Доступность"].get<bool>();

        if (available) {
            ui->RecipesList->addItem(QString::fromStdString(recipeName));
        }
    }
}

void Window::updateAllergensList()
{
    ui->AllergensList->clear();
    QStringList userAllergens = load_allergens();

    if (userAllergens.isEmpty()) {
        return;
    }

    if (!is_exists(PRODS_PATH)) {
        return;
    }
    std::ifstream in(PRODS_PATH);

    if (!in.is_open()) {
        return;
    }
    json j_prods = json::parse(in);
    in.close();

    for (auto &item : j_prods.items()) {
        std::string barcode = item.key();
        int count = item.value()["Количество"].get<int>();

        if (count <= 0) {
            continue;
        }

        std::string allergensStr = item.value()["Аллергены"].get<std::string>();

        if (allergensStr.empty() || allergensStr == "Нет данных") {
            continue;
        }
        QString qAllergensStr = QString::fromStdString(allergensStr);
        QStringList productAllergens = qAllergensStr.split(QRegularExpression("[\\s,]+"), QString::SkipEmptyParts);
        QStringList matchedAllergens;

        for (const QString &user : qAsConst(userAllergens)) {
            for (const QString &prod : qAsConst(productAllergens)) {
                if (prod.contains(user, Qt::CaseInsensitive)) {
                    if (!matchedAllergens.contains(prod, Qt::CaseInsensitive)) {
                        matchedAllergens << prod;
                    }
                }
            }
        }

        if (matchedAllergens.isEmpty()) {
            continue;
        }
        QString productName = QString::fromStdString(item.value()["Название"].get<std::string>());
        QString brand = QString::fromStdString(item.value()["Марка"].get<std::string>());
        QString displayText = productName;

        if (!brand.isEmpty() && brand != "Нет данных") {
            displayText += QString(" (%1)").arg(brand);
        }
        displayText += QString(" – %1").arg(matchedAllergens.join(", "));
        QListWidgetItem *listItem = new QListWidgetItem(displayText);
        listItem->setToolTip(QString::fromStdString(barcode));
        ui->AllergensList->addItem(listItem);
    }
}

void Window::onAllergenItemClicked(QListWidgetItem *item) {
    if (!item) {
        return;
    }
    QString barcode = item->toolTip();

    if (barcode.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось определить штрих-код продукта");
        return;
    }
    std::string barcodeStr = barcode.toStdString();


    if (!is_in(barcodeStr)) {
        QMessageBox::warning(this, "Ошибка", "Продукт не найден в базе");
        return;
    }
    int count = get_product_count(barcodeStr);

    if (count <= 0) {
        QMessageBox::information(this, "Информация", "Этот продукт уже удалён (количество = 0)");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Удаление продукта",
        QString("Вы действительно хотите полностью удалить продукт \%1\?")
            .arg(item->text().split(" –").first()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }
    del_prod(barcodeStr);

    updateRecipesList();
    updateAllergensList();
    updateSelfLifeList();

    ui->EnterLabel->setText("Продукт удалён (обнулён)");
    ui->EnterLabel->setStyleSheet("color: red;");
    m_successTimer->start(2000);
}

void Window::onRecipeClicked(QListWidgetItem *item) {
    if (!item) {
        return;
    }

    QString recipeName = item->text();

    if (recipeName.isEmpty()) {
        return;
    }
    std::ifstream in(RECIPES_PATH);

    if (!in.is_open()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось открыть файл рецептов");
        return;
    }
    json j_recipes = json::parse(in);
    in.close();

    std::string recipeNameStd = recipeName.toStdString();

    if (!j_recipes.contains(recipeNameStd)) {
        QMessageBox::warning(this, "Ошибка", "Рецепт не найден");
        return;
    }
    const auto &recipe = j_recipes[recipeNameStd];
    const auto &ingredients = recipe["Ингредиенты"];

    bool enough = true;
    QString missingList;

    for (auto &ingr : ingredients.items()) {
        std::string ingrName = ingr.key();
        int needed = ingr.value()["Количество"].get<int>();

        std::ifstream p_in(PRODS_PATH);

        if (!p_in.is_open()) {
            enough = false;
            missingList += QString::fromStdString(ingrName) + " (файл продуктов не открыт)\n";
            continue;
        }
        json j_prods = json::parse(p_in);
        p_in.close();

        int totalAvailable = 0;
        for (auto &prod : j_prods.items()) {
            if (iequals(prod.value()["Название"].get<std::string>(), ingrName)) {
                totalAvailable += prod.value()["Общий объём"].get<int>();
            }
        }
        if (totalAvailable < needed) {
            enough = false;
            missingList += QString::fromStdString(ingrName) + " (недостаточно: нужно " + QString::number(needed) + ", есть " + QString::number(totalAvailable) + ")\n";
        }
    }

    if (!enough) {
        QMessageBox::warning(this, "Недостаточно продуктов", missingList);
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Приготовление",
                                                              QString("Вы хотите приготовить \"%1\"?\nБудут списаны необходимые продукты").arg(recipeName),
                                                              QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) {
        return;
    }

    for (auto &ingr : ingredients.items()) {
        std::string ingrName = ingr.key();
        int needed = ingr.value()["Количество"].get<int>();

        std::ifstream p_in(PRODS_PATH);
        json j_prods = json::parse(p_in);
        p_in.close();

        std::vector<std::string> barcodes;
        for (auto &prod : j_prods.items()) {
            if (iequals(prod.value()["Название"].get<std::string>(), ingrName)) {
                barcodes.push_back(prod.key());
            }
        }

        int remaining = needed;
        for (const auto &bcode : barcodes) {
            if (remaining <= 0) {
                break;
            }
            int available = j_prods[bcode]["Общий объём"].get<int>();
            int toSubtract = std::min(remaining, available);

            if (toSubtract > 0) {
                reduce_prod(bcode, toSubtract);
                remaining -= toSubtract;
            }
        }
    }

    QMessageBox::information(this, "Готово", "Продукты списаны. Приятного аппетита!");

    updateRecipesList();
    updateAllergensList();
    updateSelfLifeList();
}

void Window::onRecipesManageClicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Управление рецептами");

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    QPushButton *addBtn = new QPushButton("Добавить рецепт");
    QPushButton *editBtn = new QPushButton("Изменить рецепт");
    QPushButton *delBtn = new QPushButton("Удалить рецепт");
    QPushButton *closeBtn = new QPushButton("Закрыть");

    layout->addWidget(addBtn);
    layout->addWidget(editBtn);
    layout->addWidget(delBtn);
    layout->addWidget(closeBtn);

    connect(addBtn, &QPushButton::clicked, &dialog, [this, &dialog]() {
        dialog.accept();
        addRecipeDialog();
    });
    connect(editBtn, &QPushButton::clicked, &dialog, [this, &dialog]() {
        dialog.accept();
        editRecipeDialog();
    });
    connect(delBtn, &QPushButton::clicked, &dialog, [this, &dialog]() {
        dialog.accept();
        deleteRecipeDialog();
    });
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void Window::addRecipeDialog() {
    bool ok;
    QString recipeName = QInputDialog::getText(this, "Новый рецепт", "Введите название рецепта:", QLineEdit::Normal, "", &ok);

    if (!ok || recipeName.isEmpty()) {
        return;
    }
    std::ifstream in(RECIPES_PATH);
    json j_recipes;

    if (in.is_open()) {
        j_recipes = json::parse(in);
        in.close();
    }
    else {
        j_recipes = json::object();
    }

    if (j_recipes.contains(recipeName.toStdString())) {
        QMessageBox::warning(this, "Ошибка", "Рецепт с таким названием уже существует");
        return;
    }
    std::map<std::string, int> ingredients;

    while (true) {
        QString ingrName = QInputDialog::getText(this, "Ингредиент", "Введите название продукта:", QLineEdit::Normal, "", &ok);

        if (!ok || ingrName.isEmpty()) {
            break;
        }
        int quantity = QInputDialog::getInt(this, "Количество", "Введите необходимое количество (в мл/г/шт):", 100, 1, 10000, 1, &ok);

        if (!ok) {
            break;
        }
        ingredients[ingrName.toStdString()] = quantity;

        QMessageBox::StandardButton cont = QMessageBox::question(this, "Добавить ещё?", "Добавить следующий ингредиент?", QMessageBox::Yes | QMessageBox::No);

        if (cont != QMessageBox::Yes) {
            break;
        }
    }

    if (ingredients.empty()) {
        QMessageBox::warning(this, "Ошибка", "Рецепт не может быть без ингредиентов");
        return;
    }
    j_recipes[recipeName.toStdString()]["Ингредиенты"] = ingredients;
    j_recipes[recipeName.toStdString()]["Доступность"] = false;

    for (auto &ingr : ingredients) {
        j_recipes[recipeName.toStdString()]["Ингредиенты"][ingr.first] = {{"Количество", ingr.second}, {"Наличие", false}};
    }

    std::ofstream out(RECIPES_PATH);
    out << std::setw(4) << j_recipes;
    out.close();

    recalculate_recipe(recipeName.toStdString());(recipeName.toStdString());
    updateRecipesList();
    updateSelfLifeList();

    QMessageBox::information(this, "Успех", "Рецепт добавлен");
}

void Window::editRecipeDialog() {
    std::ifstream in(RECIPES_PATH);

    if (!in.is_open()) {
        QMessageBox::warning(this, "Ошибка", "Нет файла рецептов");
        return;
    }
    json j_recipes = json::parse(in);
    in.close();

    if (j_recipes.empty()) {
        QMessageBox::information(this, "Нет рецептов", "Нет ни одного рецепта для редактирования");
        return;
    }
    QStringList recipeNames;
    for (auto &item : j_recipes.items())
        recipeNames << QString::fromStdString(item.key());

    bool ok;
    QString oldName = QInputDialog::getItem(this, "Выберите рецепт", "Рецепт:", recipeNames, 0, false, &ok);

    if (!ok || oldName.isEmpty()) {
        return;
    }
    QDialog editDialog(this);
    editDialog.setWindowTitle("Редактирование рецепта");
    QVBoxLayout *mainLayout = new QVBoxLayout(&editDialog);

    QHBoxLayout *nameLayout = new QHBoxLayout();
    QLabel *nameLabel = new QLabel("Название рецепта:");
    QLineEdit *nameEdit = new QLineEdit(oldName);

    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(nameEdit);
    mainLayout->addLayout(nameLayout);

    QListWidget *ingredientsList = new QListWidget();
    ingredientsList->setSelectionMode(QAbstractItemView::SingleSelection);
    mainLayout->addWidget(ingredientsList);

    std::map<std::string, int> ingredientsMap;

    for (auto &ingr : j_recipes[oldName.toStdString()]["Ингредиенты"].items()) {
        QString name = QString::fromStdString(ingr.key());
        int qty = ingr.value()["Количество"].get<int>();
        ingredientsMap[ingr.key()] = qty;
        ingredientsList->addItem(QString("%1 - %2 мл/г/шт").arg(name).arg(qty));
    }

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Добавить");
    QPushButton *removeBtn = new QPushButton("Удалить");
    QPushButton *modifyBtn = new QPushButton("Изменить");
    QPushButton *saveBtn = new QPushButton("Сохранить");
    QPushButton *cancelBtn = new QPushButton("Отмена");

    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(removeBtn);
    btnLayout->addWidget(modifyBtn);
    btnLayout->addWidget(saveBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    auto updateList = [&]() {
        ingredientsList->clear();
        for (auto &pair : ingredientsMap) {
            const std::string &name = pair.first;
            int qty = pair.second;
            ingredientsList->addItem(QString("%1 - %2 мл/г/шт").arg(QString::fromStdString(name)).arg(qty));
        }
    };

    connect(addBtn, &QPushButton::clicked, &editDialog, [&]() {
        QString ingrName = QInputDialog::getText(&editDialog, "Новый ингредиент", "Название продукта:");

        if (ingrName.isEmpty()) {
            return;
        }
        int qty = QInputDialog::getInt(&editDialog, "Количество", "Количество (мл/г/шт):", 100, 1, 10000, 1);

        if (qty <= 0) {
            return;
        }

        ingredientsMap[ingrName.toStdString()] = qty;
        updateList();
    });

    connect(removeBtn, &QPushButton::clicked, &editDialog, [&]() {
        QListWidgetItem *cur = ingredientsList->currentItem();

        if (!cur) {
            return;
        }
        QString text = cur->text();
        QString ingrName = text.left(text.indexOf(" -"));
        ingredientsMap.erase(ingrName.toStdString());

        updateList();
    });

    connect(modifyBtn, &QPushButton::clicked, &editDialog, [&]() {
        QListWidgetItem *cur = ingredientsList->currentItem();

        if (!cur) {
            return;
        }
        QString text = cur->text();
        QString oldIngrName = text.left(text.indexOf(" -"));
        int oldQty = ingredientsMap[oldIngrName.toStdString()];
        QString newIngrName = QInputDialog::getText(&editDialog, "Изменение ингредиента", "Название продукта:", QLineEdit::Normal, oldIngrName);

        if (newIngrName.isEmpty()) {
            newIngrName = oldIngrName;
        }
        int newQty = QInputDialog::getInt(&editDialog, "Количество", "Количество (мл/г/шт):", oldQty, 1, 10000, 1);

        if (newQty <= 0) {
            newQty = oldQty;
        }

        if (newIngrName.toStdString() != oldIngrName.toStdString()) {
            ingredientsMap.erase(oldIngrName.toStdString());
        }
        ingredientsMap[newIngrName.toStdString()] = newQty;
        updateList();
    });

    connect(saveBtn, &QPushButton::clicked, &editDialog, [&]() {
        QString newRecipeName = nameEdit->text().simplified();

        if (newRecipeName.isEmpty()) {
            QMessageBox::warning(&editDialog, "Ошибка", "Название рецепта не может быть пустым");
            return;
        }

        if (ingredientsMap.empty()) {
            QMessageBox::warning(&editDialog, "Ошибка", "Рецепт должен содержать хотя бы один ингредиент");
            return;
        }
        std::string oldNameStd = oldName.toStdString();
        std::string newNameStd = newRecipeName.toStdString();

        if (newNameStd != oldNameStd) {
            j_recipes.erase(oldNameStd);
        }
        j_recipes[newNameStd]["Ингредиенты"] = ingredientsMap;
        j_recipes[newNameStd]["Доступность"] = false;

        for (auto &pair : ingredientsMap) {
            const std::string &name = pair.first;
            int qty = pair.second;
            j_recipes[newNameStd]["Ингредиенты"][name] = {{"Количество", qty}, {"Наличие", false}};
        }

        std::ofstream out(RECIPES_PATH);
        out << std::setw(4) << j_recipes;
        out.close();

        QMessageBox::information(this, "Успех", "Рецепт изменён");
        recalculate_recipe(newNameStd);
        updateRecipesList();
        updateSelfLifeList();
        editDialog.accept();
    });

    connect(cancelBtn, &QPushButton::clicked, &editDialog, &QDialog::reject);
    editDialog.exec();
}

void Window::deleteRecipeDialog() {
    std::ifstream in(RECIPES_PATH);

    if (!in.is_open()) {
        QMessageBox::warning(this, "Ошибка", "Нет файла рецептов");
        return;
    }
    json j_recipes = json::parse(in);
    in.close();

    if (j_recipes.empty()) {
        QMessageBox::information(this, "Нет рецептов", "Нет ни одного рецепта для удаления");
        return;
    }
    QStringList recipeNames;

    for (auto &item : j_recipes.items()) {
        recipeNames << QString::fromStdString(item.key());
    }
    bool ok;
    QString recipeName = QInputDialog::getItem(this, "Удаление рецепта", "Выберите рецепт:", recipeNames, 0, false, &ok);

    if (!ok || recipeName.isEmpty()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение", QString("Удалить рецепт \"%1\"?").arg(recipeName), QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }
    j_recipes.erase(recipeName.toStdString());

    std::ofstream out(RECIPES_PATH);
    out << std::setw(4) << j_recipes;
    out.close();

    updateRecipesList();
    updateSelfLifeList();
    QMessageBox::information(this, "Успех", "Рецепт удалён");
}

void Window::updateSelfLifeList() {
    ui->SelfLifeList->clear();
    QStringList expired = get_expired_products();
    ui->SelfLifeList->addItems(expired);
}

void Window::onSelfLifeItemClicked(QListWidgetItem *item) {
    if (!item) {
        return;
    }
    QString productName = item->text();
    std::ifstream in(PRODS_PATH);
    json j_prods = json::parse(in);
    in.close();
    std::vector<std::string> expiredBarcodes;

    for (auto &prod : j_prods.items()) {
        if (prod.value()["Название"] == productName.toStdString() && is_product_expired(prod.key())) {
            expiredBarcodes.push_back(prod.key());
        }
    }

    if (expiredBarcodes.empty()) {
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(this, "Удаление просрочки",
        QString("Продукт \"%1\" просрочен.\nУдалить остаток?").arg(productName),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        for (const auto& bc : expiredBarcodes) {
            discard_expired_product(bc);
        }
        updateRecipesList();
        updateAllergensList();
        updateSelfLifeList();
        ui->EnterLabel->setText("Просроченные остатки удалены");
        ui->EnterLabel->setStyleSheet("color: red;");
        m_successTimer->start(2000);
    }
}
