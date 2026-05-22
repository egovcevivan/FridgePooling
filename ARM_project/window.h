#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QPushButton>
#include <QListWidgetItem>

#include "fixedlineedit.h"


QT_BEGIN_NAMESPACE
namespace Ui { class Window; }
QT_END_NAMESPACE

class Window : public QMainWindow
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);
    ~Window();

private slots:
    void onValidationResult(bool success, const QString& barcode);
    void onTextChanged(const QString &text);
    void onAllergenButtonClicked();
    void updateRecipesList();
    void updateAllergensList();
    void onRecipeClicked(QListWidgetItem *item);
    void onRecipesManageClicked();
    void onAllergenItemClicked(QListWidgetItem *item);
    void updateSelfLifeList();
    void onSelfLifeItemClicked(QListWidgetItem *item);

private:
    Ui::Window *ui;
    bool m_skipNextClear;
    void addRecipeDialog();
    void editRecipeDialog();
    void deleteRecipeDialog();
    void editProductParameters(const std::string &barcode);
    QTimer *m_successTimer;
    QPushButton *m_allergenButton;
    QPushButton *m_recipesManageButton;
    QListWidget *m_selfLifeList;
};

#endif
