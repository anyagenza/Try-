#include <QDebug>
#include <algorithm>
#include <fstream>
#include "gameboarddata.h"
#include <QJsonArray>
#include <QJsonParseError>
#include <QFile>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <iostream>

GameBoardData::GameBoardData(int sizeX, int sizeY, int colorCount, QObject* parent) : QAbstractListModel(parent),
    m_sizeX(sizeX), m_sizeY(sizeY), m_dimension(sizeX * sizeY), m_colorCount(colorCount), rd(), eng(rd())
{
    read(":/input.json");
    isMatch = 0;
    score = 0;
    tempScore = 0;
    std::uniform_int_distribution<> distr(0, colorCount - 1);

    for (int i = 0; i < m_sizeY; i++)
    {
        matchHorisontal.append(QList<int>());
        matchVertical.append(QList<int>());
        match.append(QList<int>());
        for (int j = 0; j < m_sizeX; j++)
        {
            matchHorisontal[i].append(int());
            matchVertical[i].append(int());
            match[i].append(int());
        }
    }

    for (int i = 0; i < m_dimension; i++) {
        int randColorKey = distr(eng);
        m_data.push_back(randColorKey);
    }
    isMatch = checkMatch(m_data);
    shuffle();
}

void GameBoardData::read(std::string inp)
{
    QFile file_obj(QString::fromStdString(inp));
    if (!file_obj.exists()) {
        exit(1);

    }
    if (!file_obj.open(QIODevice::ReadOnly|QIODevice::Text)) {

        exit(1);
    }
    QTextStream file_text(&file_obj);
    QString json_string;
    json_string = file_text.readAll();
    file_obj.close();
    QByteArray json_bytes = json_string.toLocal8Bit();
    auto json_doc = QJsonDocument::fromJson(json_bytes);
    QJsonObject json_obj = json_doc.object();
    auto result = json_obj.toVariantMap();
    m_sizeX = result["width"].toInt();
    m_sizeY = result["height"].toInt();
    QVariantList localList = result["colorNames"].toList();
    int count = 0;
    for (auto& el: localList) {
        m_colorKey[count] = el.toString();
        count++;
    }
    m_colorCount = count;
}

void GameBoardData::shuffle()
{
    std::uniform_int_distribution<> distr(0, m_colorCount - 1);
    isMatch = true;
    score = 0;
    tempScore = 0;
    emit isScoreChanged();
    beginResetModel();
    while (isMatch) {
        for(int i = 0; i < m_dimension; i++) {
            int randColorKey = distr(eng);
            m_data[i] = randColorKey;
        }
        isMatch = checkMatch(m_data);
    }
    endResetModel();
}

bool GameBoardData::ifNear(int first, int second) const
{
    return ((std::abs(first - second) == 1) || (std::abs(first - second) == m_sizeY));
}


void  GameBoardData::checkMatchHorisontal()
{
    for (int i = 0; i < m_sizeY; i++) {
        for (int j = 0; j < m_sizeX; j++) {
            int currentElement = matchHorisontal[i][j];
            int countMatch = 0;
            int first = j;
            while ((j < m_sizeX - 1) && (currentElement == matchHorisontal[i][j+1])) {
                countMatch++;
                j++;
            }
            for(int k = first; k < first + countMatch + 1; k++) {
                matchHorisontal[i][k] = countMatch + 1;
            }
        }
    }
}

void GameBoardData::checkMatchVertical()
{
    for (int i = 0; i < m_sizeX; i++) {
        for (int j = 0; j < m_sizeY; j++) {
            int currentElement = matchVertical[j][i];
            int countMatch = 0;
            int first = j;
            while ((j < m_sizeY - 1) && (currentElement == matchVertical[j + 1][i])) {
                countMatch++;
                j++;
            }
            for (int k = first; k < first + countMatch + 1; k++) {
                matchVertical[k][i] = countMatch + 1;
            }
        }
    }
}

bool GameBoardData::checkMatch(QList<int>& m_data)
{
    for (int x = 0; x < m_sizeY; x++) {
        for (int k = 0; k < m_sizeX; k++) {
            matchHorisontal[x][k] = m_data[m_sizeY*k + x];
            matchVertical[x][k] = matchHorisontal[x][k];
        }
    }
    checkMatchVertical();
    checkMatchHorisontal();
    for (int i = 0; i < m_sizeY; i++) {
        for (int j = 0; j < m_sizeX; j++) {
            match[i][j] = std::max(matchVertical[i][j], matchHorisontal[i][j]);
        }
    }
    for (int i = 0; i < m_sizeY; i++) {
        for (int j = 0; j < m_sizeX; j++) {
            if (match[i][j] >= 3) {
                return true;
            }
        }
    }
    return false;
}

void GameBoardData::setMatchToNull()
{
    for (int i = 0; i < m_sizeY; i++) {
        for (int j = 0; j < m_sizeX; j++) {
            matchVertical[i][j] = 0;
            match[i][j] = matchHorisontal[i][j] = 0;
        }
    }
}

bool GameBoardData::getIsMatch()
{
    return isMatch;
}

void GameBoardData::clear()
{
    indexForDelete.clear();
    for (int i = 0; i < m_sizeY; i++) {
        for (int j = 0; j < m_sizeX; j++) {
            if (match[i][j] >= 3) {
                indexForDelete.push_back(Inx{i ,j});
            }
        }
    }
    QList<int> q;
    for (auto el : indexForDelete) {
        q.push_back(el.x * m_sizeY + el.y);
    }
    std::sort(q.begin(), q.end());
    QMap<int,int> m;
    for (int i = 0; i < m_sizeY; i++) {
        m.insert(i,0);
    }
    while (!q.isEmpty()) {
        int forRemove = q.back();
        q.pop_back();
        beginRemoveRows(QModelIndex(), forRemove, forRemove);
        m_data.erase(m_data.begin() + forRemove);
        endRemoveRows();
        int temp = m[forRemove / m_sizeY];
        m[forRemove / m_sizeY] = temp + 1;
    }
    std::uniform_int_distribution<> distr(0, m_colorCount - 1);
    for (int i = 0; i < m_sizeY; i++) {
        for (int j = 0; j < m[i]; j++) {
            beginInsertRows(QModelIndex(), i * m_sizeY, i * m_sizeY);
            m_data.insert(i * m_sizeY, distr(eng));
            endInsertRows();
        }
    }
    emit isScoreChanged();
    setMatchToNull();
}

void GameBoardData::moveElements(int indexFirst, int indexSecond)
{
    int offsetForHorizontal = indexFirst < indexSecond ? 1 : 0;
    int offsetForVertical = indexFirst < indexSecond ? 0 : 1;
    if (ifNear(indexFirst, indexSecond)) {
        if (std::abs(indexSecond - indexFirst) == 1) {
            beginMoveRows(QModelIndex(),indexFirst,indexFirst,QModelIndex(),indexSecond + offsetForHorizontal);
            endMoveRows();
        } else if (std::abs(indexSecond - indexFirst) == m_sizeY) {
            beginMoveRows(QModelIndex(),indexFirst,indexFirst,QModelIndex(),indexSecond);
            endMoveRows();
            beginMoveRows(QModelIndex(), indexSecond + offsetForVertical,
                          indexSecond + offsetForVertical, QModelIndex(), indexFirst + offsetForVertical);
            endMoveRows();
        }
        int temp = m_data[indexFirst];
        m_data[indexFirst] = m_data[indexSecond];
        m_data[indexSecond] = temp;
        for (int i = 0; i < m_sizeY; i++) {
            for (int j = 0; j < m_sizeX; j++) {
                if (match[i][j] >= 3) {
                    tempScore++;
                }
            }
        }
        score = tempScore;
        emit isScoreChanged();
    }
    isMatch = checkMatch(m_data);
    emit isScoreChanged();
}

void GameBoardData::swapElements(int indexFirst, int indexSecond)
{
    QList<int> copyData  = {m_data.begin(), m_data.end()};
    int temp = copyData[indexFirst];
    copyData[indexFirst] = copyData[indexSecond];
    copyData[indexSecond] = temp;

    if (checkMatch(copyData)) {
        moveElements(indexFirst, indexSecond);
    }
    else {
        emit noMatch(indexFirst, indexSecond);
    }
    //ifGameOver();
}

void GameBoardData::clearMatchAgain()
{
    if (isMatch) {
        isMatch = checkMatch(m_data);
        for (int i = 0; i < m_sizeY; i++) {
            for (int j = 0; j < m_sizeX; j++) {
                if (match[i][j] >= 3) {
                    tempScore++;
                }
            }
        }
        score = tempScore;
        emit isScoreChanged();
        clear();
    }
}

QVariant GameBoardData::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= m_data.count()) {
        return QVariant();
    }
    switch(role) {
    case Qt::DisplayRole:
        return m_colorKey[m_data.value(row)];
    }
    return QVariant();
}

int GameBoardData::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_data.count();
}

int GameBoardData::getScore()
{
    return score;
}

int GameBoardData::getSizeX()
{
    return m_sizeX;
}

int GameBoardData::getSizeY()
{
    return m_sizeY;
}

bool GameBoardData::ifGameOver()
{
    QList<QList<int>> checkList;
    for (int i = 0; i < m_sizeY; i++)
    {
        checkList.append(QList<int>());
        for (int j = 0; j < m_sizeX; j++)
        {
            checkList[i].append(int());
        }
    }
    for (int x = 0; x < m_sizeX; x++) {
        for (int k = 0; k < m_sizeY; k++) {
            checkList[k][x] = m_data[m_sizeY * x + k];
        }
    }
    qDebug() << "hi";
    int head = 0;
    int tail = 3;
    int up = 0;
    int down = 2;
    for (int i = 0; i < m_sizeY - 1; i++) {
        head = 0;
        tail = 3;
        for (int j = 0; j < m_sizeX - 3; j++) {
            if (checkList[i][j] == checkList[i][j+1] == checkList[i+1][j+2]) {
                qDebug() << "1"<< " " << i << " " << j;
                return false;
            }
            if (checkList[i][j] == checkList[i + 1][j + 1] == checkList[i][j + 2]) {
                qDebug() << "2"<< " " << i << " " << j;
                return false;
            }
            if (checkList[i + 1][j] == checkList[i][j+1] == checkList[i][j+2]) {
                qDebug() << "3"<< " " << i << " " << j;
                return false;
            }
            if (checkList[i][j] == checkList[i][j+1] == checkList[i][j + 3]) {
                qDebug() << "4"<< " " << i << " " << j;
                return false;
            }
            if (checkList[i][j] == checkList[i][j + 2] == checkList[i][j + 3]) {
                qDebug() << "5" << " " << i << " " << j;
                return false;
            }
        }
    }
    exit(1);
    qDebug() << "gameOver";
    return true;


}
