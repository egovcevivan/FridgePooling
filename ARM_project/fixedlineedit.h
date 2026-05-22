#ifndef FIXEDLINEEDIT_H
#define FIXEDLINEEDIT_H

#include <QLineEdit>
#include <QRegularExpressionValidator>


class FixedLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit FixedLineEdit(QWidget *parent = nullptr);

signals:
    void validationResult(bool success, const QString& barcode);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onTextChanged(const QString &text);

private:
    void applyStyle(bool isValid);
    bool m_lastValidState = false;
};

#endif
