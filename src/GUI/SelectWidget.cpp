#include "SelectWidget.h"
#include "../StaticClass/Global.h"
#include "../StaticClass/XinjiaoyuNetwork.h"
#include "../StaticClass/Setting.h"
#include "../StaticClass/CallAndroidNativeComponent.h"
#include "../StaticClass/QRCodeScanner.h"
#include "../GUI/TemplateDetailWidget.h"
#include "../GUI/SearchWidget.h"

SelectWidget::SelectWidget(QWidget *parent)
    : QWidget{ parent }
{
    mainLayout = new QVBoxLayout(this);
    listWidgetTabWidget = new QTabWidget(this);
    OKButton = new QPushButton(QStringLiteral("确定"), this);
    searchButton = new QPushButton(QStringLiteral("搜索"), this);
    previousPageButton = new QPushButton(QStringLiteral("上一页"), this);
    nextPageButton = new QPushButton(QStringLiteral("下一页"), this);
    scanQRCodeButton = new QPushButton(QStringLiteral("扫码"), this);
    templateCodeLineEdit = new QLineEdit(this);

#ifdef Q_OS_ANDROID
    const QString filePath { QStringLiteral("assets:/templateList/") };
#else
    const QString filePath { QStringLiteral(":/template/templateList/") };
#endif
    loadFromFile(filePath);

    templateCodeLineEdit->setPlaceholderText(QStringLiteral("题卡编号"));
    OKButton->setEnabled(false);

    QFont smallFont;
    const auto defaultFontPixelSize{QWidget::font().pixelSize()};
    if(Setting::smallFontPointSize != 0)
    {
        smallFont.setPointSize(Setting::smallFontPointSize);
    }
    else
    {
        smallFont.setPixelSize(defaultFontPixelSize / 2);
    }
    listWidgetTabWidget->setFont(smallFont);

    previousPageButton->setVisible(false);
    nextPageButton->setVisible(false);

    auto addHBoxLayoutWithTwoWidget{[](QWidget * widget1, QWidget * widget2)
    {
        auto layout{new QHBoxLayout};
        layout->addWidget(widget1);
        layout->addWidget(widget2);
        return layout;
    }};
    mainLayout->addLayout(addHBoxLayoutWithTwoWidget(scanQRCodeButton, searchButton));
    mainLayout->addWidget(listWidgetTabWidget);
    mainLayout->addLayout(addHBoxLayoutWithTwoWidget(previousPageButton, nextPageButton));
    mainLayout->addWidget(templateCodeLineEdit);
    mainLayout->addWidget(OKButton);


    connect(OKButton, &QPushButton::clicked, this, &SelectWidget::OKButtonPushed);
    connect(searchButton, &QPushButton::clicked, this, &SelectWidget::searchButtonPushed);
    connect(previousPageButton, &QPushButton::clicked, this, &SelectWidget::toPreviousPage);
    connect(nextPageButton, &QPushButton::clicked, this, &SelectWidget::toNextPageButton);
    connect(scanQRCodeButton, &QPushButton::clicked, this, &SelectWidget::scanQRCode);
}

void SelectWidget::loadFromFile(const QString &dirPath)
{
    for(auto i{0}; i < listWidgetTabWidget->count(); ++i)
    {
        listWidgetTabWidget->removeTab(i);
        listWidgetTabWidget->widget(i)->deleteLater();
    }
    templateCodeFinder.clear();

    const auto subjects{ QStringList() << QStringLiteral("语文") << QStringLiteral("数学") << QStringLiteral("英语") << QStringLiteral("物理") << QStringLiteral("化学") << QStringLiteral("生物") };
    const auto fileListNames{ QStringList() << QStringLiteral("templateList_chinese") << QStringLiteral("templateList_mathematics") << QStringLiteral("templateList_english") << QStringLiteral("templateList_physics") << QStringLiteral("templateList_chemistry") << QStringLiteral("templateList_biography") };

    auto addListWidget{[this](const QString & filePath, const QString & name)
    {
        auto tempListWidget{new QListWidget};
        QFile file { filePath };
        if(file.exists())
        {
            file.open(QFile::ReadOnly);
            while (!file.atEnd())
            {
                QString tempTemplateName{file.readLine()};
                tempTemplateName.resize(tempTemplateName.size() - 1);
                tempTemplateName.squeeze();
                auto item{ new QListWidgetItem(tempTemplateName, tempListWidget) };

                QString tempTemplateCode{file.readLine()};
                tempTemplateCode.resize(tempTemplateCode.size() - 1);
                tempTemplateCode.squeeze();

                templateCodeFinder.insert(item, tempTemplateCode);
            }
            file.close();
        }
        QScroller::grabGesture(tempListWidget->viewport(), QScroller::TouchGesture);
        tempListWidget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        tempListWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        tempListWidget->setAutoScroll(false);
        listWidgetTabWidget->addTab(tempListWidget, name);
        connect(tempListWidget, &QListWidget::itemSelectionChanged, [this, tempListWidget]
        {
            this->itemSelectionChanged(tempListWidget->currentItem());
        });
        return tempListWidget;
    }};

    const QDir allDir {dirPath + QStringLiteral("all")};
    const QDir latestDir{dirPath + QStringLiteral("latest")};

    auto callFunc{[&addListWidget, &fileListNames, &subjects](const QDir & dir, int index)
    {
        addListWidget(dir.filePath(fileListNames.at(index)), subjects.at(index));
    }};

    if(Setting::listLatestTemplatePreferentially && latestDir.exists())
    {
        for(auto i(0); i < 6; ++i)
        {
            callFunc(latestDir, i);
        }
    }
    else if(allDir.exists())
    {
        for(auto i(0); i < 6; ++i)
        {
            callFunc(allDir, i);
        }
    }

    addListWidget(Global::tempPath() + QDir::separator() + QStringLiteral("templateList_undefined"), QStringLiteral("undefined"));
}

void SelectWidget::toPreviousPage()
{
    static_cast<QListWidget *>(listWidgetTabWidget->currentWidget())
    ->verticalScrollBar()
    ->setSliderPosition(
        static_cast<QListWidget *>(listWidgetTabWidget->currentWidget())
        ->verticalScrollBar()
        ->sliderPosition() -
        listWidgetTabWidget->height());
}

void SelectWidget::toNextPageButton()
{
    static_cast<QListWidget *>(listWidgetTabWidget->currentWidget())
    ->verticalScrollBar()
    ->setSliderPosition(
        static_cast<QListWidget *>(listWidgetTabWidget->currentWidget())
        ->verticalScrollBar()
        ->sliderPosition() +
        listWidgetTabWidget->height());
}

void SelectWidget::searchButtonPushed()
{
    auto searchWidget{new SearchWidget};
    searchWidget->setAttribute(Qt::WA_DeleteOnClose);
    searchWidget->setAttribute(Qt::WA_QuitOnClose, false);
    connect(searchWidget, &SearchWidget::searchFinished, [this, searchWidget](const QString & templateCode)
    {
        this->templateCodeLineEdit->setText(templateCode);
        this->OKButtonPushed();
        searchWidget->close();
    });
    searchWidget->show();
}

void SelectWidget::OKButtonPushed()
{
    const auto analysisWebRawData{getTemplateCodeData(templateCodeLineEdit->text().trimmed())};
    if(analysisWebRawData.isEmpty())
    {
        return;
    }

#ifdef Q_OS_ANDROID
    emit searchFinished(analysisWebRawData);
#else // Q_OS_ANDROID
    auto templateDetailWidget {new TemplateDetailWidget(analysisWebRawData)};
    templateDetailWidget->setAttribute(Qt::WA_DeleteOnClose);
    templateDetailWidget->setAttribute(Qt::WA_QuitOnClose, false);
    templateDetailWidget->show();
#endif // Q_OS_ANDROID
}

AnalysisWebRawData SelectWidget::getTemplateCodeData(const QString &templateCode)
{
    if(templateCode.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral(""), QStringLiteral("题卡编号不能为空"));
        return AnalysisWebRawData();
    }
    QByteArray webRawData;
    QString templateName;
    auto addTemplate{[&templateName, &templateCode, this]
        {
            auto namedUndefinedListWidget{static_cast<QListWidget*>(listWidgetTabWidget->widget(6))};
            const QString data{templateName + QStringLiteral("\n") + templateCode + QStringLiteral("\n")};
            QFile f(Global::tempPath() + QDir::separator() + QStringLiteral("templateList_undefined"));
            f.open(QFile::ReadWrite | QFile::Append);
            f.write(data.toUtf8());
            f.close();
            //NOTE 和总数目相关,undefined在第7个
            auto newItem{new QListWidgetItem(templateName, static_cast<QListWidget*>(listWidgetTabWidget->widget(6)))};
            templateCodeFinder.insert(newItem, templateCode);
            namedUndefinedListWidget->setCurrentItem(newItem);
            namedUndefinedListWidget->scrollToItem(newItem);
            listWidgetTabWidget->setCurrentIndex(6);
        }};
#ifdef Q_OS_ANDROID
    QFile file { QStringLiteral("assets:/templateData/") + templateCode };
#else
    QFile file { QStringLiteral(":/template/templateData/") + templateCode };
#endif
    QFile fileTemp { Global::tempPath() + QDir::separator() + QStringLiteral("TemplateFile") + QDir::separator() + templateCode };
    if (file.exists())
    {
        file.open(QFile::ReadOnly);
        webRawData = file.readAll();
        templateName = QJsonDocument::fromJson(webRawData).object().value(QStringLiteral("data")).toObject().value(QStringLiteral("templateName")).toString();
        file.close();
    }
    else if (fileTemp.exists())
    {
        fileTemp.open(QFile::ReadOnly);
        webRawData = fileTemp.readAll();
        fileTemp.close();
        templateName = QJsonDocument::fromJson(webRawData).object().value(QStringLiteral("data")).toObject().value(QStringLiteral("templateName")).toString();
        auto namedUndefinedListWidget{static_cast<QListWidget*>(listWidgetTabWidget->widget(6))};
        if(namedUndefinedListWidget->findItems(templateName, Qt::MatchExactly).isEmpty())
        {
            addTemplate();
        }
    }
    else
    {
        try
        {
            webRawData = XinjiaoyuNetwork::getTemplateCodeData(templateCode);
        }
        catch (const std::exception &e)
        {
            QMessageBox::critical(this, QStringLiteral("critical"), e.what());
            return AnalysisWebRawData();
        }
        fileTemp.open(QFile::WriteOnly);
        fileTemp.write(webRawData);
        fileTemp.close();
        templateName = QJsonDocument::fromJson(webRawData).object().value(QStringLiteral("data")).toObject().value(QStringLiteral("templateName")).toString();

        addTemplate();
    }

    return AnalysisWebRawData(webRawData, templateName, templateCode);
}

void SelectWidget::itemSelectionChanged(QListWidgetItem *item)
{
    this->templateCodeLineEdit->setText(templateCodeFinder.value(item));
    OKButton->setEnabled(true);
}

void SelectWidget::scanQRCode()
{
    scanQRCodeButton->setEnabled(false);

    QMessageBox msgBox1;
    msgBox1.setText(QStringLiteral("请选择扫码方式:"));
    const auto takePhotoBtn{ msgBox1.addButton(QStringLiteral("拍照"), QMessageBox::AcceptRole) };
    const auto selectFileBtn{ msgBox1.addButton(QStringLiteral("选择文件"), QMessageBox::AcceptRole) };
    msgBox1.addButton(QStringLiteral("取消"), QMessageBox::RejectRole)->setVisible(false);
    msgBox1.exec();

    QByteArray decodeResult;

    if(msgBox1.clickedButton() == takePhotoBtn)
    {
#ifdef Q_OS_ANDROID
        auto image {CallAndroidNativeComponent::takePhoto()};
        if(image.isNull())
        {
            scanQRCodeButton->setEnabled(true);
            return;
        }
        QMessageBox msgBox2;
        msgBox2.setText(QStringLiteral("解析中..."));
        msgBox2.show();
        try
        {
            decodeResult = QRCodeScanner::scanQRCode(image, "JPEG");
        }
        catch (const std::exception &e)
        {
            QMessageBox::critical(Q_NULLPTR, QStringLiteral("critical"), e.what());
            scanQRCodeButton->setEnabled(true);
            return;
        }

        msgBox2.close();
#endif // Q_OS_ANDROID
    }
    else if(msgBox1.clickedButton() == selectFileBtn)
    {
        const auto imagePath{QFileDialog::getOpenFileName(this, QStringLiteral("选择文件"), QString(), QStringLiteral("Images (*.bmp *.gif *.jpg *.jpeg *.png *.tiff *.pbm *.pgm *.ppm *.xbm *.xpm)"))};
        if(imagePath.isEmpty())
        {
            scanQRCodeButton->setEnabled(true);
            return;
        }
        QMessageBox msgBox2;
        msgBox2.setText(QStringLiteral("解析中..."));
        msgBox2.show();
        try
        {
            decodeResult = QRCodeScanner::scanQRCode(imagePath, "JPEG");
        }
        catch (const std::exception &e)
        {
            QMessageBox::critical(Q_NULLPTR, QStringLiteral("critical"), e.what());
            scanQRCodeButton->setEnabled(true);
            return;
        }
        msgBox2.close();
    }
    else
    {
        scanQRCodeButton->setEnabled(true);
        return;
    }

    if(decodeResult.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("warning"), QStringLiteral("扫描失败\n"
                             "请确保二维码清晰可见"));
        scanQRCodeButton->setEnabled(true);
        return;
    }
    qDebug() << decodeResult;
    this->templateCodeLineEdit->setText(decodeResult);
    if(Setting::getTemplateCodeDataAfterScanQRCodeSuccessfully)
    {
        this->getTemplateCodeData(decodeResult);
    }
    scanQRCodeButton->setEnabled(true);
}
