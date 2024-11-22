#ifndef SEARCHROOM_H
#define SEARCHROOM_H

#include <QWidget>
#include <QTcpSocket>


namespace Ui {
class SearchRoom;
}

class SearchRoom : public QWidget
{
    Q_OBJECT

public:
    explicit SearchRoom(QString name, QTcpSocket* socket,int score, QWidget *parent = nullptr);
    ~SearchRoom();

private slots:
    void on_addBtn_clicked();

private:
    Ui::SearchRoom *ui;
    QString m_name;
    QTcpSocket* m_socket;
    int m_score;
};

#endif // SEARCHROOM_H
