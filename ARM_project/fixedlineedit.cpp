#include <QKeyEvent>

#include "fixedlineedit.h"


FixedLineEdit::FixedLineEdit(QWidget *parent) : QLineEdit(parent)
{
    setValidator(new QRegularExpressionValidator(QRegularExpression("\\d{13}"), this));
    connect(this, &QLineEdit::textChanged, this, &FixedLineEdit::onTextChanged);
}

void FixedLineEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QString text = this->text();
        bool ok = (text.length() == 13);

        if (ok) {
            applyStyle(true);
            emit validationResult(true, text);
            clear();
        }
        else {
            applyStyle(false);
            emit validationResult(false, text);
        }
        return;
    }
    QLineEdit::keyPressEvent(event);
}

void FixedLineEdit::onTextChanged(const QString &text)
{
    Q_UNUSED(text);
    m_lastValidState = false;
}

void FixedLineEdit::applyStyle(bool isValid)
{
    if (isValid) {
        m_lastValidState = true;
    }
    else {
        m_lastValidState = false;
    }
}
