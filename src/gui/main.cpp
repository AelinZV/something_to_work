#include "app/RepositoryFactory.h"
#include "gui/ResponsiveLayout.h"
#include "gui/QuestMapMarkers.h"
#include "service/GameService.h"

#include <QApplication>
#include <QCloseEvent>
#include <QColor>
#include <QFile>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QResizeEvent>
#include <QTextDocument>
#include <QStringList>
#include <QList>
#include <QMainWindow>
#include <QMessageBox>
#include <QMouseEvent>
#include <QEvent>
#include <QToolTip>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QVariant>

#include <algorithm>
#include <array>
#include <clocale>
#include <functional>
#include <map>
#include <memory>
#include <utility>
#include <stdexcept>
#include <string>
#include <vector>

// No X11 API is used directly; keep our domain Status safe if a platform header did include Xlib.
#ifdef Status
#undef Status
#endif

namespace {
using namespace border;

QString qs(const std::string& s) { return QString::fromStdString(s); }
QString signedHtml(const std::string& s) {
    return qs(s).toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br>"));
}

const char* kStyle = R"CSS(
    QWidget { background: #151d20; color: #e9e4d9; font-family: "Noto Sans", "DejaVu Sans", sans-serif; font-size: 13px; }
    QMainWindow, QWidget#root { background: #101719; }
    QFrame#card, QGroupBox { background: #202b2d; border: 1px solid #354646; border-radius: 10px; }
    QGroupBox { font-weight: 600; color: #e6c48b; margin-top: 12px; padding-top: 18px; }
    QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0px 5px; }
    QLabel#brand { font-size: 22px; font-weight: 700; letter-spacing: 1px; color: #e8c88c; }
    QLabel#hint { color: #9eadac; font-size: 12px; }
    QLabel#section { color: #e6c48b; font-size: 15px; font-weight: 700; }
    QLabel#statLabel { color: #d8d1c2; font-size: 12px; }
    QPushButton { border-radius: 7px; background: #364a48; border: 1px solid #58706a; padding: 8px 9px; color: #f0ecdf; font-weight: 600; }
    QPushButton:hover { background: #49645b; border-color: #d2b279; }
    QPushButton:pressed { background: #253631; }
    QPushButton#primary { background: #826332; border-color: #b99455; color: #fff4d7; }
    QPushButton#primary:hover { background: #997642; }
    QTreeWidget, QTextBrowser, QPlainTextEdit { background: #172225; border: 1px solid #405453; border-radius: 7px; padding: 7px; color: #ede7d8; selection-background-color: #826437; }
    QTreeWidget::item { padding: 6px; }
    QTreeWidget::item:selected { background: #675331; color: #fff4dc; }
    QHeaderView::section { background: #202b2d; color: #e6c48b; }
    QProgressBar { background: #111b1e; border: 1px solid #455551; border-radius: 5px; text-align: center; min-height: 15px; color: white; }
    QProgressBar::chunk { background: #a3474b; border-radius: 4px; }
    QProgressBar#xp::chunk { background: #b68a47; }
    QTabWidget::pane { border: 1px solid #43534f; border-radius: 6px; }
    QTabBar::tab { background: #253434; padding: 10px 20px; border-radius: 5px; margin-right: 3px; }
    QTabBar::tab:selected { background: #795c37; color: white; }
    QSplitter::handle { background: #111a1c; width: 7px; }
    QScrollBar:vertical { background: #182326; width: 12px; }
    QScrollBar::handle:vertical { background: #52665f; min-height: 25px; border-radius: 5px; }
)CSS";

QFrame* card(QWidget* parent = nullptr) {
    auto* f = new QFrame(parent);
    f->setObjectName(QStringLiteral("card"));
    return f;
}
QLabel* sectionTitle(const QString& title) {
    auto* l = new QLabel(title);
    l->setObjectName(QStringLiteral("section"));
    return l;
}

class CoverBanner final : public QWidget {
    QPixmap cover_;
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::SmoothPixmapTransform, true);
        p.fillRect(rect(), QColor("#253431"));
        if (!cover_.isNull()) {
            const double factor = std::max(double(width()) / cover_.width(),
                                           double(height()) / cover_.height());
            const double sw = width() / factor;
            const double sh = height() / factor;
            const QRectF source((cover_.width() - sw) / 2.0,
                                (cover_.height() - sh) / 2.0, sw, sh);
            p.drawPixmap(QRectF(rect()), cover_, source);
        }
        QLinearGradient shade(0, 0, width(), 0);
        shade.setColorAt(0, QColor(10, 17, 18, 225));
        shade.setColorAt(0.65, QColor(10, 17, 18, 35));
        shade.setColorAt(1, QColor(10, 17, 18, 75));
        p.fillRect(rect(), shade);
        p.setPen(QColor("#f5e4be"));
        QFont title = p.font();
        title.setBold(true);
        title.setPixelSize(std::clamp(width() / 34, 18, 36));
        p.setFont(title);
        p.drawText(QRect(23, 5, width() - 46, height() - 25),
                   int(Qt::AlignLeft | Qt::AlignVCenter) | int(Qt::TextWordWrap),
                   QStringLiteral("КОНТРАКТЫ\nПОГРАНИЧЬЯ"));
        p.setPen(QColor("#d0c5ac"));
        QFont subtitle = p.font(); subtitle.setBold(false); subtitle.setPixelSize(12);
        p.setFont(subtitle);
        p.drawText(QRect(26, height()-25, width()-40, 18), Qt::AlignLeft,
                   QStringLiteral("Земля древних проклятий · Истории охотника"));
    }
public:
    explicit CoverBanner(const QString& imagePath, QWidget* parent = nullptr)
        : QWidget(parent), cover_(imagePath) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        setMinimumHeight(105);
    }
};

class WorldMap final : public QWidget {
    int current_{0};
    int selected_{0};
    int focusedQuest_{0};
    ui::MapQuestGroups markers_{};
    std::array<std::array<std::size_t,4>,4> clickOffsets_{};
    std::pair<int,int> hover_{-1,-1};
    std::function<void(int)> clicked_;
    std::function<void(int)> questClicked_;

    std::array<QPointF,4> positions() const {
        const QRectF a = rect().adjusted(15,20,-15,-20);
        return {{{a.left()+a.width()*0.5, a.top()+a.height()*0.53},
                 {a.left()+a.width()*0.18, a.top()+a.height()*0.80},
                 {a.left()+a.width()*0.82, a.top()+a.height()*0.78},
                 {a.left()+a.width()*0.51, a.top()+a.height()*0.20}}};
    }
    // Четыре компактных маркера около каждой точки. При изменении размера
    // карты их позиции пересчитываются относительно координаты локации.
    QRectF badgeRect(int location, int status, std::size_t count) const {
        const QPointF pt=positions()[static_cast<std::size_t>(location)];
        const double dx=location==1 ? -33.0 : location==2 ? 33.0 : location==3 ? -65.0 : 65.0;
        const double startY=location==3 ? -6.0 : location==0 ? -48.0 : -64.0;
        const double cy=pt.y()+startY+status*18.0;
        const double w=count==1 ? 23.0 : (count<10 ? 32.0 : 41.0);
        // Плашки всегда остаются внутри видимой области карты.
        const double cx=std::clamp(pt.x()+dx,w/2+3.0,double(width())-w/2-3.0);
        const double y=std::clamp(cy-9.5,2.0,std::max(2.0,double(height())-21.0));
        return QRectF(cx-w/2,y,w,19);
    }
    std::pair<int,int> badgeAt(const QPointF& pos) const {
        for(int location=0;location<4;++location) {
            for(int status=0;status<4;++status) {
                const auto& quests=markers_[static_cast<std::size_t>(location)]
                                           [static_cast<std::size_t>(status)];
                if(!quests.empty() && badgeRect(location,status,quests.size()).contains(pos))
                    return {location,status};
            }
        }
        return {-1,-1};
    }
    static QColor statusColor(int status) {
        static const std::array<QColor,4> colors{
            QColor("#6689a5"),  // Доступен — серо-синий
            QColor("#c49645"),  // Принят — янтарный
            QColor("#53a4ae"),  // Расследован — бирюзовый
            QColor("#56936c")   // Завершён — зелёный
        };
        return colors.at(static_cast<std::size_t>(status));
    }
    static QString statusSymbol(int status) {
        static const std::array<QString,4> symbols{
            QStringLiteral("!"),QStringLiteral("▶"),
            QStringLiteral("?"),QStringLiteral("✓")};
        return symbols.at(static_cast<std::size_t>(status));
    }
    QString badgeTooltip(int location,int status) const {
        const auto& quests=markers_[static_cast<std::size_t>(location)]
                                   [static_cast<std::size_t>(status)];
        const Status s=static_cast<Status>(status);
        QString text=qs(locationName(location))+QStringLiteral(" · ")+qs(statusName(s))+
                     QStringLiteral(" (%1)").arg(static_cast<qulonglong>(quests.size()));
        for(std::size_t i=0;i<quests.size() && i<12;++i)
            text+=QStringLiteral("\n#%1: %2").arg(quests[i].id).arg(qs(quests[i].title));
        if(quests.size()>12) text+=QStringLiteral("\n...ещё %1").arg(static_cast<qulonglong>(quests.size()-12));
        text+=QStringLiteral("\nНажмите, чтобы выбрать заказ");
        if(quests.size()>1) text+=QStringLiteral(" (повторное нажатие — следующий)");
        return text;
    }
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if(event->button()!=Qt::LeftButton) {QWidget::mousePressEvent(event);return;}
        const auto [location,status]=badgeAt(event->position());
        if(location>=0) {
            selected_=location;
            auto& quests=markers_[static_cast<std::size_t>(location)]
                                 [static_cast<std::size_t>(status)];
            auto& offset=clickOffsets_[static_cast<std::size_t>(location)]
                                      [static_cast<std::size_t>(status)];
            const int contractId=quests[offset % quests.size()].id;
            offset=(offset+1)%quests.size();
            focusedQuest_=contractId;
            update();
            if(clicked_) clicked_(location);
            if(questClicked_) questClicked_(contractId);
            return;
        }
        const auto locs=positions();
        for(int i=0;i<4;++i) {
            const QPointF delta=event->position()-locs[static_cast<std::size_t>(i)];
            if(delta.x()*delta.x()+delta.y()*delta.y()<=28*28) {
                selected_=i;update();
                if(clicked_) clicked_(i);
                return;
            }
        }
        QWidget::mousePressEvent(event);
    }
    void mouseMoveEvent(QMouseEvent* event) override {
        const auto hit=badgeAt(event->position());
        if(hit!=hover_) {
            hover_=hit;
            if(hit.first>=0) {
                setCursor(Qt::PointingHandCursor);
                QToolTip::showText(event->globalPosition().toPoint(),
                                   badgeTooltip(hit.first,hit.second),this);
            } else {
                unsetCursor();
                QToolTip::hideText();
            }
        }
        QWidget::mouseMoveEvent(event);
    }
    void leaveEvent(QEvent* event) override {
        hover_={-1,-1};unsetCursor();QToolTip::hideText();
        QWidget::leaveEvent(event);
    }
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(),QColor("#162526"));
        const auto points=positions();
        p.setPen(QPen(QColor("#596e61"),2,Qt::DashLine));
        for(int i=1;i<4;++i)p.drawLine(points[0],points[static_cast<std::size_t>(i)]);
        const std::array<QString,4> names{
            QStringLiteral("Сероводье"),QStringLiteral("Топь"),
            QStringLiteral("Мельница"),QStringLiteral("Чёрный бор")};
        for(int i=0;i<4;++i) {
            const QPointF point=points[static_cast<std::size_t>(i)];
            if(i==selected_) {
                p.setPen(QPen(QColor("#efdca5"),2));
                p.setBrush(Qt::NoBrush);
                p.drawEllipse(point,17,17);
            }
            p.setBrush(QColor(i==current_?"#e6b36f":"#a2bbaa"));
            p.setPen(QPen(QColor("#243530"),2));
            p.drawEllipse(point,8,8);

            // Цвет обозначает состояние заказов, символ — дополнительный
            // визуальный признак. Количество одинаковых статусов не теряется.
            for(int status=0;status<4;++status) {
                const auto& quests=markers_[static_cast<std::size_t>(i)]
                                           [static_cast<std::size_t>(status)];
                if(quests.empty())continue;
                const QRectF badge=badgeRect(i,status,quests.size());
                const bool focused=std::any_of(quests.begin(),quests.end(),[this](const ui::MapQuest& q){
                    return q.id==focusedQuest_;
                });
                p.setBrush(statusColor(status));
                p.setPen(QPen(QColor(focused?"#fff2c7":"#142022"),focused?2:1));
                p.drawRoundedRect(badge,7,7);
                QFont font=p.font();font.setPixelSize(11);font.setBold(true);p.setFont(font);
                p.setPen(QColor("#ffffff"));
                const QString label=statusSymbol(status)+
                    (quests.size()==1 ? QString() : QString::number(static_cast<qulonglong>(quests.size())));
                p.drawText(badge,Qt::AlignCenter,label);
            }
            p.setPen(QColor("#eee9dc"));
            QFont font=p.font();font.setPixelSize(11);font.setBold(false);p.setFont(font);
            const double dy=i==3 ? -39 : 17;
            p.drawText(QRectF(point.x()-62,point.y()+dy,124,20),Qt::AlignHCenter,names[i]);
        }
    }
public:
    explicit WorldMap(QWidget* parent=nullptr):QWidget(parent) {
        setMinimumHeight(175);
        setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
        setMouseTracking(true);
        setToolTip(QStringLiteral("Нажмите на локацию для путешествия или на цветной маркер для выбора заказа"));
    }
    int selected() const {return selected_;}
    void selectLocation(int id) {selected_=std::clamp(id,0,3);update();}
    void currentLocation(int id) {current_=std::clamp(id,0,3);update();}
    void highlightContract(int id) {focusedQuest_=id;update();}
    void setQuestMarkers(ui::MapQuestGroups groups) {
        markers_=std::move(groups);
        clickOffsets_={}; hover_={-1,-1};
        update();
    }
    void onSelected(std::function<void(int)> fn) {clicked_=std::move(fn);}
    void onQuestSelected(std::function<void(int)> fn) {questClicked_=std::move(fn);}
};

class GameWindow final : public QMainWindow {
    std::unique_ptr<IGameRepository> repository_;
    GameService game_;
    bool demo_{false};
    bool compact_{false};
    bool initialized_{false};
    int lastStoryContract_{-1};
    QWidget* root_{};
    CoverBanner* cover_{};
    QLabel* brand_{},*storage_{},*location_{},*statLine_{};
    QProgressBar* hpBar_{},*xpBar_{};
    QStackedWidget* body_{};
    QWidget* widePage_{};
    QSplitter* splitter_{};
    QTabWidget* compactPage_{};
    QWidget* contractsPanel_{},*storyPanel_{},*actionsPanel_{};
    QTreeWidget* contractTree_{};
    WorldMap* map_{};
    QTextBrowser* storyView_{};
    QPlainTextEdit* logView_{};
    std::map<int,QTreeWidgetItem*> contractRows_;

    template <typename Func>
    void perform(Func&& func) {
        try {
            const std::string message = func();
            if (!message.empty()) log(message);
            refresh();
            if (game_.selected() && game_.storyCompleted()) {
                // Shown only once per actual action (not on every resize).
                log("Все доступные контракты выполнены. Пограничье ждёт новых историй.");
            }
        } catch (const std::exception& ex) {
            log(std::string("Ошибка: ")+ex.what());
            QMessageBox::warning(this,QStringLiteral("Действие невозможно"),qs(ex.what()));
        }
    }

    int selectedContract() const {
        auto* item = contractTree_->currentItem();
        if (!item) throw std::runtime_error("Выберите контракт в списке");
        const int id = item->data(0,Qt::UserRole).toInt();
        if (id<=0) throw std::runtime_error("Выберите конкретный заказ внутри локации");
        return id;
    }
    void log(const std::string& message) {
        if (logView_->document()->blockCount()>700) logView_->clear();
        logView_->appendPlainText(QStringLiteral("› ")+qs(message));
        logView_->verticalScrollBar()->setValue(logView_->verticalScrollBar()->maximum());
    }
    void addButton(QGridLayout* grid,int idx,const QString& title,
                   const std::function<void()>& fn, bool important=false) {
        auto* b = new QPushButton(title);
        b->setMinimumHeight(36);
        if (important) b->setObjectName(QStringLiteral("primary"));
        grid->addWidget(b,idx/2,idx%2);
        connect(b,&QPushButton::clicked,this,fn);
    }
    QGridLayout* actionGroup(QVBoxLayout* root,const QString& name) {
        auto* frame = new QGroupBox(name);
        auto* grid = new QGridLayout(frame);
        grid->setSpacing(7);grid->setContentsMargins(9,12,9,9);
        root->addWidget(frame);
        return grid;
    }
    void makePanels() {
        contractsPanel_=card();
        auto* boardOuter = new QVBoxLayout(contractsPanel_);
        boardOuter->setContentsMargins(6,6,6,6);
        auto* boardScroll = new QScrollArea();
        boardScroll->setWidgetResizable(true);
        boardScroll->setFrameShape(QFrame::NoFrame);
        auto* boardBody = new QWidget();
        auto* boardLayout = new QVBoxLayout(boardBody);
        boardLayout->setContentsMargins(6,6,6,6);
        boardLayout->setSpacing(9);
        boardLayout->addWidget(sectionTitle(QStringLiteral("ДОСКА ЗАКАЗОВ")));
        auto* boardHint = new QLabel(QStringLiteral("Заказы сгруппированы по локациям"));
        boardHint->setWordWrap(true);
        boardLayout->addWidget(boardHint);
        contractTree_=new QTreeWidget();
        contractTree_->setHeaderHidden(true);
        contractTree_->setIndentation(15);
        contractTree_->setMinimumHeight(140);
        boardLayout->addWidget(contractTree_,5);
        boardLayout->addWidget(sectionTitle(QStringLiteral("КАРТА ПОГРАНИЧЬЯ")));
        map_=new WorldMap();
        boardLayout->addWidget(map_,4);
        auto* legend=new QLabel(QStringLiteral(
            "<span style='color:#6689a5'>● !</span> Доступен &nbsp; "
            "<span style='color:#c49645'>● ▶</span> Принят &nbsp; "
            "<span style='color:#53a4ae'>● ?</span> Расследован &nbsp; "
            "<span style='color:#56936c'>● ✓</span> Завершён"));
        legend->setWordWrap(true);
        legend->setTextFormat(Qt::RichText);
        legend->setToolTip(QStringLiteral("Цвет и символ — статус. Цифра на маркере — количество заказов с таким статусом."));
        boardLayout->addWidget(legend);
        boardScroll->setWidget(boardBody);
        boardOuter->addWidget(boardScroll);
        connect(contractTree_,&QTreeWidget::itemSelectionChanged,this,[this]{
            int id=0;
            try{id=selectedContract();}catch(...){}
            map_->highlightContract(id);
            refreshStory();
        });
        map_->onSelected([this](int location){
            log("Выбрана точка: "+locationName(location)+". Для перехода нажмите «Путешествовать».");
        });
        map_->onQuestSelected([this](int contractId){
            const auto it=contractRows_.find(contractId);
            if(it==contractRows_.end())return;
            contractTree_->setCurrentItem(it->second);
            contractTree_->scrollToItem(it->second);
            map_->highlightContract(contractId);
            log("Выбран контракт с карты: "+game_.getContract(contractId).title());
        });

        storyPanel_=card();
        auto* storyLayout = new QVBoxLayout(storyPanel_);
        storyLayout->setContentsMargins(12,12,12,12);
        storyLayout->setSpacing(9);
        storyLayout->addWidget(sectionTitle(QStringLiteral("ЛЕТОПИСЬ КОНТРАКТА")));
        storyView_=new QTextBrowser();
        storyView_->setOpenExternalLinks(false);
        storyView_->setReadOnly(true);
        storyView_->setMinimumHeight(120);
        storyLayout->addWidget(storyView_,5);
        storyLayout->addWidget(sectionTitle(QStringLiteral("ЖУРНАЛ СОБЫТИЙ")));
        logView_=new QPlainTextEdit();
        logView_->setReadOnly(true);
        logView_->setMinimumHeight(90);
        storyLayout->addWidget(logView_,3);

        actionsPanel_=card();
        auto* actionRoot = new QVBoxLayout(actionsPanel_);
        actionRoot->setContentsMargins(11,11,11,11);
        actionRoot->addWidget(sectionTitle(QStringLiteral("ДЕЙСТВИЯ ОХОТНИКА")));
        auto* scroll=new QScrollArea();
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        auto* inside = new QWidget();
        auto* groups=new QVBoxLayout(inside);
        groups->setContentsMargins(0,0,3,0);
        groups->setSpacing(8);

        auto* heroActions=actionGroup(groups,QStringLiteral("Персонаж"));
        addButton(heroActions,0,QStringLiteral("Создать"),[this]{newHero();});
        addButton(heroActions,1,QStringLiteral("Загрузить"),[this]{chooseHero();});
        addButton(heroActions,2,QStringLiteral("Навыки"),[this]{learnSkill();});
        addButton(heroActions,3,QStringLiteral("Инвентарь"),[this]{showInventory();});
        addButton(heroActions,4,QStringLiteral("Удалить"),[this]{deleteHero();});

        auto* questActions=actionGroup(groups,QStringLiteral("Контракт"));
        addButton(questActions,0,QStringLiteral("Принять заказ"),[this]{perform([&]{return game_.acceptContract(selectedContract());});},true);
        addButton(questActions,1,QStringLiteral("Путешествовать"),[this]{perform([&]{game_.travel(map_->selected());return "Вы прибыли: "+locationName(game_.hero().location());});});
        addButton(questActions,2,QStringLiteral("Расследовать"),[this]{investigate();});
        addButton(questActions,3,QStringLiteral("Начать бой"),[this]{perform([&]{return game_.startBattle(selectedContract());});},true);

        auto* fightActions=actionGroup(groups,QStringLiteral("Бой и знаки"));
        const std::array<std::pair<const char*,Action>,9> attacks{{
            {"Удар мечом",Action::Strike},{"Сильный удар",Action::Heavy},
            {"Огненный знак",Action::Igni},{"Защитный знак",Action::Quen},
            {"Магическая ловушка",Action::Yrden},{"Нанести масло",Action::Oil},
            {"Мирный ритуал",Action::Ritual},{"Отступить",Action::Flee},
            {"Эликсир в бою",Action::Potion}
        }};
        for (int i=0;i<static_cast<int>(attacks.size());++i) {
            const auto item=attacks[static_cast<size_t>(i)];
            addButton(fightActions,i,QString::fromUtf8(item.first),[this,action=item.second]{
                perform([&]{return game_.turn(selectedContract(),action);});
            });
        }

        auto* supplies=actionGroup(groups,QStringLiteral("Алхимия и отдых"));
        addButton(supplies,0,QStringLiteral("Лечиться (+40 HP)"),[this]{perform([&]{return game_.drinkPotion();});});
        addButton(supplies,1,QStringLiteral("Отдых (10 крон)"),[this]{perform([&]{return game_.rest();});});
        addButton(supplies,2,QStringLiteral("Торговец"),[this]{shop();});
        addButton(supplies,3,QStringLiteral("Варить эликсир"),[this]{perform([&]{return game_.craft(1);});});
        addButton(supplies,4,QStringLiteral("Сделать масло"),[this]{perform([&]{return game_.craft(2);});});

        groups->addStretch(1);
        scroll->setWidget(inside);
        actionRoot->addWidget(scroll);
        actionRoot->addWidget(new QLabel(QStringLiteral("Прогресс сохраняется после каждого действия")));
    }
    void setupBody() {
        widePage_=new QWidget();
        auto* wideLayout=new QHBoxLayout(widePage_);
        wideLayout->setContentsMargins(0,0,0,0);
        splitter_=new QSplitter(Qt::Horizontal);
        splitter_->setChildrenCollapsible(false);
        wideLayout->addWidget(splitter_);
        splitter_->addWidget(contractsPanel_);
        splitter_->addWidget(storyPanel_);
        splitter_->addWidget(actionsPanel_);
        splitter_->setStretchFactor(0,3);
        splitter_->setStretchFactor(1,5);
        splitter_->setStretchFactor(2,3);
        splitter_->setSizes({300,550,300});

        compactPage_=new QTabWidget();
        body_=new QStackedWidget();
        body_->addWidget(widePage_);
        body_->addWidget(compactPage_);
    }
    void setCompact(bool compact) {
        if (compact_==compact) return;
        compact_=compact;
        if (compact) {
            // Reparent the live panels, keeping all selection, scroll and gameplay state.
            for (QWidget* panel : {contractsPanel_, storyPanel_, actionsPanel_}) panel->setParent(nullptr);
            compactPage_->addTab(contractsPanel_,QStringLiteral("Заказы и карта"));
            compactPage_->addTab(storyPanel_,QStringLiteral("История"));
            compactPage_->addTab(actionsPanel_,QStringLiteral("Действия"));
            body_->setCurrentWidget(compactPage_);
        } else {
            while (compactPage_->count()>0) {
                QWidget* panel=compactPage_->widget(0);
                compactPage_->removeTab(0);
                panel->setParent(nullptr);
            }
            for (QWidget* panel : {contractsPanel_, storyPanel_, actionsPanel_}) splitter_->addWidget(panel);
            splitter_->setSizes({300,550,300});
            body_->setCurrentWidget(widePage_);
        }
    }
    void refreshStory() {
        if (!game_.selected()) return;
        const int previous=lastStoryContract_;
        int selected;
        try {selected=selectedContract();}
        catch (...) {
            storyView_->setHtml(QStringLiteral("<p>Выберите контракт на доске.</p>"));
            lastStoryContract_=-1;
            return;
        }
        const auto& c=game_.getContract(selected);
        const auto& p=game_.getProgress(selected);
        const int oldScroll=storyView_->verticalScrollBar()->value();
        QString html=QStringLiteral("<h2 style='color:#e9c88b;'>%1</h2>").arg(signedHtml(c.title()));
        html+=QStringLiteral("<p style='line-height:1.45;'>%1</p>").arg(signedHtml(c.story()));
        html+=QStringLiteral("<p><b>Место:</b> %1<br><b>Награда:</b> %2 крон · %3 опыта<br><b>Статус:</b> %4</p>")
                  .arg(signedHtml(locationName(c.location())))
                  .arg(c.reward()).arg(c.experience()).arg(signedHtml(statusName(p.status)));
        if (p.status==Status::Investigated || p.status==Status::Completed) {
            html+=QStringLiteral("<hr><h3 style='color:#c8aa75;'>Расследование</h3><p>%1</p><p>Противник: %2<br>Слабость: %3</p>")
                  .arg(signedHtml(c.clue()),signedHtml(c.monster().name()),signedHtml(c.monster().weakness()));
        }
        if (p.fighting) {
            html+=QStringLiteral("<p><b>Бой:</b> %1 / %2 HP противника<br>Сосредоточение: %3 · Щит: %4</p>")
                  .arg(p.monsterHp).arg(c.monster().hp()).arg(p.focus).arg(p.shield);
        }
        if (p.status==Status::Completed) {
            html+=QStringLiteral("<hr><h3 style='color:#c8aa75;'>Исход</h3><p>%1</p>")
                  .arg(signedHtml(p.outcome));
        }
        storyView_->setHtml(html);
        storyView_->verticalScrollBar()->setValue(previous==selected ? oldScroll : 0);
        lastStoryContract_=selected;
    }
    void refresh() {
        if (!game_.selected())return;
        const Hero& h=game_.hero();
        statLine_->setText(QStringLiteral("%1 · ур. %2 · атака %3 · защита %4 · золото %5 · очки навыков %6")
                     .arg(qs(h.name())).arg(h.level()).arg(h.attack()).arg(h.defense()).arg(h.gold()).arg(h.skillPoints()));
        hpBar_->setMaximum(h.maxHp());hpBar_->setValue(h.hp());
        hpBar_->setFormat(QStringLiteral("HP  %1 / %2").arg(h.hp()).arg(h.maxHp()));
        xpBar_->setMaximum(h.level()*100);xpBar_->setValue(h.experience());
        xpBar_->setFormat(QStringLiteral("Опыт  %1 / %2").arg(h.experience()).arg(h.level()*100));
        location_->setText(QStringLiteral("Текущее место: %1").arg(qs(locationName(h.location()))));
        map_->currentLocation(h.location());

        int previousId=0;
        try {previousId=selectedContract();}catch(...){}
        const QSignalBlocker blocker(contractTree_);
        contractTree_->clear();contractRows_.clear();
        std::map<int,QTreeWidgetItem*> roots;
        std::vector<ui::MapQuest> mapQuests;
        for (const auto& c:game_.contracts()) {
            auto it=roots.find(c.location());
            if(it==roots.end()) {
                auto* parent=new QTreeWidgetItem(contractTree_);
                parent->setText(0,qs(locationName(c.location())));
                parent->setData(0,Qt::UserRole,0);
                parent->setFlags(parent->flags() & ~Qt::ItemIsSelectable);
                roots[c.location()]=parent;it=roots.find(c.location());
            }
            auto* row=new QTreeWidgetItem(it->second);
            row->setData(0,Qt::UserRole,c.id());
            const auto& p=game_.getProgress(c.id());
            mapQuests.push_back({c.id(),c.location(),p.status,c.title()});
            row->setText(0,QStringLiteral("[%1] %2 (#%3)")
                         .arg(qs(statusName(p.status)),qs(c.title())).arg(c.id()));
            row->setToolTip(0,QStringLiteral("Место: %1\nНаграда: %2 крон")
                             .arg(qs(locationName(c.location()))).arg(c.reward()));
            contractRows_[c.id()]=row;
        }
        for (const auto& root:roots)root.second->setExpanded(true);
        map_->setQuestMarkers(ui::groupMapQuests(mapQuests));
        if (previousId && contractRows_.count(previousId)) contractTree_->setCurrentItem(contractRows_.at(previousId));
        else if (!contractRows_.empty()) contractTree_->setCurrentItem(contractRows_.begin()->second);
        try {map_->highlightContract(selectedContract());}
        catch (...) {map_->highlightContract(0);}
        refreshStory();
    }
    void newHero() {
        bool ok=false;
        auto name=QInputDialog::getText(this,QStringLiteral("Новый охотник"),QStringLiteral("Имя:"),QLineEdit::Normal,QStringLiteral("Каэрн"),&ok);
        if(ok) perform([&]{game_.createHero(name.toStdString());return "Создан герой: "+game_.hero().name();});
    }
    void chooseHero() {
        try {
            auto heroes=game_.listHeroes();
            if(heroes.empty())throw std::runtime_error("Нет сохранённых персонажей");
            QStringList choices;
            for(const auto& h:heroes)choices<<QStringLiteral("#%1 · %2 (ур. %3)").arg(h.id()).arg(qs(h.name())).arg(h.level());
            bool ok=false;
            const QString chosen=QInputDialog::getItem(this,QStringLiteral("Загрузить сохранение"),QStringLiteral("Персонаж:"),choices,0,false,&ok);
            if(ok) {
                const int index=choices.indexOf(chosen);
                if(index>=0)perform([&]{game_.selectHero(heroes[static_cast<size_t>(index)].id());lastStoryContract_=-1;return "Загружен персонаж: "+game_.hero().name();});
            }
        }catch(const std::exception& ex){QMessageBox::warning(this,QStringLiteral("Ошибка"),qs(ex.what()));}
    }
    void deleteHero() {
        if (!game_.selected()) return;
        const int id=game_.hero().id();
        if(QMessageBox::question(this,QStringLiteral("Удаление сохранения"),
              QStringLiteral("Удалить персонажа и его прогресс? Это необратимо."))!=QMessageBox::Yes)return;
        perform([&]{
            game_.deleteHero(id);
            const auto heroes=game_.listHeroes();
            if(!heroes.empty())game_.selectHero(heroes.front().id());
            else game_.createHero("Каэрн");
            lastStoryContract_=-1;
            return "Сохранение удалено";
        });
    }
    void investigate() {
        const QStringList choices{QStringLiteral("Изучить следы"),QStringLiteral("Расспросить свидетелей")};
        bool ok=false;
        const QString value=QInputDialog::getItem(this,QStringLiteral("Расследование"),QStringLiteral("Метод:"),choices,0,false,&ok);
        if(ok)perform([&]{return game_.investigate(selectedContract(),choices.indexOf(value)+1);});
    }
    void learnSkill() {
        if(!game_.selected())return;
        const auto& h=game_.hero();
        const QStringList choices{
            QStringLiteral("Мастерство меча (%1): +3 к атаке").arg(h.swordSkill()),
            QStringLiteral("Живучесть (%1): +20 HP").arg(h.vitalitySkill()),
            QStringLiteral("Мощь знаков (%1): сильнее знаки").arg(h.signSkill())};
        bool ok=false;
        const QString value=QInputDialog::getItem(this,QStringLiteral("Навыки · свободно %1").arg(h.skillPoints()),
                                                  QStringLiteral("Выберите улучшение:"),choices,0,false,&ok);
        if(ok)perform([&]{return game_.learnSkill(choices.indexOf(value)+1);});
    }
    void shop() {
        const QStringList choices{QStringLiteral("Красный эликсир · 25 крон"),QStringLiteral("Серебряное масло · 30 крон"),QStringLiteral("Лечебные травы · 8 крон")};
        bool ok=false;
        const QString value=QInputDialog::getItem(this,QStringLiteral("Торговец"),QStringLiteral("Купить:"),choices,0,false,&ok);
        if(ok)perform([&]{return game_.purchase(choices.indexOf(value)+1);});
    }
    void showInventory() {
        if(!game_.selected())return;
        QString text;
        for (int id=1;id<=3;++id)
            text+=QStringLiteral("%1: %2\n").arg(qs(itemName(id))).arg(game_.countItem(id));
        QMessageBox::information(this,QStringLiteral("Инвентарь"),text);
    }
protected:
    void resizeEvent(QResizeEvent* ev) override {
        QMainWindow::resizeEvent(ev);
        if (!initialized_)return;
        const auto policy=border::ui::policyFor(width(),height());
        cover_->setFixedHeight(policy.coverHeight);
        map_->setMinimumHeight(policy.mapMinimumHeight);
        setCompact(policy.compact);
    }
public:
    explicit GameWindow(std::unique_ptr<IGameRepository> repo,bool demo)
       :repository_(std::move(repo)),game_(*repository_),demo_(demo) {
        setWindowTitle(QStringLiteral("Контракты Пограничья · Qt 6"));
        resize(1440,900);
        setMinimumSize(740,560);
        setStyleSheet(QString::fromUtf8(kStyle));
        root_=new QWidget(); root_->setObjectName(QStringLiteral("root"));
        setCentralWidget(root_);
        auto* base=new QVBoxLayout(root_);
        base->setContentsMargins(15,11,15,12);base->setSpacing(9);
        auto* heading=new QHBoxLayout();
        brand_=new QLabel(QStringLiteral("ПОГРАНИЧЬЕ"));brand_->setObjectName(QStringLiteral("brand"));
        storage_=new QLabel(demo?QStringLiteral("ДЕМО · без сохранений"):QStringLiteral("POSTGRESQL · автосохранение"));
        storage_->setObjectName(QStringLiteral("hint"));
        heading->addWidget(brand_);heading->addStretch(1);heading->addWidget(storage_);
        base->addLayout(heading);
        cover_=new CoverBanner(QString::fromUtf8(BORDERLAND_ASSET_DIR) + QStringLiteral("/cover.png"));
        base->addWidget(cover_);
        auto* status=card();
        auto* statusLayout=new QVBoxLayout(status);
        statusLayout->setContentsMargins(12,8,12,8);statusLayout->setSpacing(5);
        statLine_=new QLabel();statLine_->setObjectName(QStringLiteral("statLabel"));
        statLine_->setWordWrap(true);
        statusLayout->addWidget(statLine_);
        auto* meters=new QHBoxLayout();
        hpBar_=new QProgressBar();xpBar_=new QProgressBar();
        xpBar_->setObjectName(QStringLiteral("xp"));
        meters->addWidget(hpBar_);meters->addWidget(xpBar_);
        statusLayout->addLayout(meters);
        location_=new QLabel();location_->setObjectName(QStringLiteral("hint"));
        statusLayout->addWidget(location_);
        base->addWidget(status);
        makePanels();setupBody();
        base->addWidget(body_,1);
        initialized_=true;
        const auto policy=border::ui::policyFor(width(),height());
        cover_->setFixedHeight(policy.coverHeight);
        map_->setMinimumHeight(policy.mapMinimumHeight);
        setCompact(policy.compact);
        const auto heroes=game_.listHeroes();
        if(heroes.empty())game_.createHero("Каэрн");
        else game_.selectHero(heroes.front().id());
        refresh();
        log("Выберите заказ в списке. Подробности прокручиваются, текст переносится по ширине окна.");
        log("На карте можно выбрать локацию; затем нажмите «Путешествовать».");
        log("Лечение: красный эликсир (+40 HP) или отдых в Сероводье за 10 крон.");
        if (demo_)log("Демо-режим: PostgreSQL не используется, данные исчезают после выхода.");
    }
};
}

int main(int argc, char** argv) {
    std::setlocale(LC_ALL, "");
    QApplication app(argc,argv);
    bool demo=false,smoke=false;
    for(int i=1;i<argc;++i){
        const std::string arg(argv[i]);
        if(arg=="--demo")demo=true;
        else if(arg=="--smoke-test")smoke=true;
        else {
            QMessageBox::critical(nullptr,QStringLiteral("Параметры запуска"),
                                  QStringLiteral("Использование: borderland_gui [--demo] [--smoke-test]"));
            return 2;
        }
    }

    try {
        auto repo=border::openRepository(demo);
        GameWindow window(std::move(repo),demo);
        window.show();
        if(smoke){
            QTimer::singleShot(0,&app,[&]{
                window.resize(800,640);
                window.resize(1540,960);
                app.quit();
            });
        }
        return app.exec();
    } catch(const std::exception& ex) {
        QMessageBox::critical(nullptr,QStringLiteral("Не удалось запустить игру"),
                              qs(ex.what())+QStringLiteral("\nПроверьте PostgreSQL и sql/init.sql"));
        return 2;
    }
}
