#include "mainwindow.h"
#include <QtGui>
#include <ctime>

const QString WIDGET_NAME = "ImageView";
const QString WINDOW_TITLE = "GraphCut";
const int TOOLTIP_STRETCH = 5000;
const int VIEWER_WIDTH = 800;
const int VIEWER_HEIGHT = 600;

GrabCutNS::Image<GrabCutNS::Color>* create_image_array_from_QImage(const QImage &img)
{
	int w = img.width();
	int h = img.height();
	QRgb clr;
	GrabCutNS::Real R, G, B;
	GrabCutNS::Image<GrabCutNS::Color> *imgArr;
	imgArr = new GrabCutNS::Image<GrabCutNS::Color>(w, h);
	for (int y=0; y<h; ++y)
	{
		for (int x=0; x<w; ++x)
		{
			clr = img.pixel(x, y);
			R = static_cast<GrabCutNS::Real>(qRed(clr) / 255.0f);
			G = static_cast<GrabCutNS::Real>(qGreen(clr) / 255.0f);
			B = static_cast<GrabCutNS::Real>(qBlue(clr) / 255.0f);
			(*imgArr)(x,y) = GrabCutNS::Color(R, G, B);
		}
	}
	return imgArr;
}

void copy_image_array_to_QImage(const GrabCutNS::Image<GrabCutNS::Color>* imgArr, QImage &qImg)
{
	int w = imgArr->width();
	int h = imgArr->height();
	Q_ASSERT(qImg.width() == w && qImg.height() == h);

	int r, g, b;
	for (int y=0; y<h; ++y)
	{
		for (int x=0; x<w; ++x)
		{
			r = static_cast<int>((*imgArr)(x,y).r * 255.0f);
			g = static_cast<int>((*imgArr)(x,y).g * 255.0f);
			b = static_cast<int>((*imgArr)(x,y).b * 255.0f);
			qImg.setPixel(x,y, qRgb(r,g,b));
		}
	}
}

void copy_image_array_to_QImage(const GrabCutNS::Image<GrabCutNS::Real>* imgArr, QImage &qImg)
{
	int w = imgArr->width();
	int h = imgArr->height();
	Q_ASSERT(qImg.width() == w && qImg.height() == h);

	int luminance;
	for (int y=0; y<h; ++y)
	{
		for (int x=0; x<w; ++x)
		{
			luminance = static_cast<int>((*imgArr)(x,y) * 255.0f);
			qImg.setPixel(x,y, qRgb(luminance,luminance,luminance));
		}
	}
}

void copy_alpha_array_to_QImage(const GrabCutNS::Image<GrabCutNS::Real>* alphaArr, QImage &qImg, bool mask =  true);

void copy_alpha_array_to_QImage(const GrabCutNS::Image<GrabCutNS::Real>* alphaArr, QImage &qImg, bool mask)
{
	int w = alphaArr->width();
	int h = alphaArr->height();
	Q_ASSERT(qImg.width() == w && qImg.height() == h);

	QRgb clr;
	int alpha;
	for (int y=0; y<h; ++y)
	{
		for (int x=0; x<w; ++x)
		{
			clr = qImg.pixel(x, y);
			alpha = static_cast<int>((*alphaArr)(x,y) * 255.0f);
			if (mask)
			{
				qImg.setPixel(x,y, qRgba(0,0,0, alpha));
			}
			else
			{
				if (alpha > 128)
					qImg.setPixel(x,y, qRgba(0,0,0,0));
			}
		}
	}
}

MainWindow::MainWindow()
{
	init();
}

MainWindow::~MainWindow()
{
}

void MainWindow::init()
{
	srand((unsigned int)time(0));

	setupUi();
	createActions();
	createMenus();
	createToolBars();
	createStatusBar();	

	initParameters();
}

void MainWindow::initSystem()
{
	initParameters();

	// let size of other images same as original image
	for (int i=1; i<VM_MAX; ++i)
	{
		mImages[i] = mImages[VM_IMAGE];
	}
	mImgMask = QImage(mImages[VM_IMAGE].size(), QImage::Format_ARGB32);
	mImgMask.fill(0);
}

void MainWindow::initParameters()
{
	mViewMode = VM_IMAGE;
	mSelectionMode = SM_NONE;
	mRefining = false;
	mInitialized = false;
	mShowMask = false;

	if (mImages->isNull())
		mImgView->setFixedSize(VIEWER_WIDTH, VIEWER_HEIGHT);
	else
		mImgView->setFixedSize(mImages[VM_IMAGE].size());
}

void MainWindow::setupUi()
{
	mImgView = new QWidget;
	mImgView->setObjectName(WIDGET_NAME);
	this->setCentralWidget(mImgView);
	mImgView->installEventFilter(this);
	this->layout()->setSizeConstraint(QLayout::SetFixedSize);

	mProgressBar = new QProgressBar(this);

	setWindowTitle(WINDOW_TITLE);
}

QSize MainWindow::sizeHint() const
{
	return mImages[VM_IMAGE].size();
}

void MainWindow::createActions()
{
	// file menu
	mOpenAct = new QAction(QIcon(":/images/open.png"), tr("&Open Obj..."), this);
	mOpenAct->setShortcut(QKeySequence::Open);
	mOpenAct->setToolTip(tr("Open an image file"));
	connect(mOpenAct, SIGNAL(triggered()), this, SLOT(open()));

	mSaveAsImageAct = new QAction(QIcon(":/images/save.png"), tr("&Save As Image..."), this);
	mSaveAsImageAct->setShortcut(QKeySequence::Save);
	mSaveAsImageAct->setToolTip(tr("Save the segmented image"));
	connect(mSaveAsImageAct, SIGNAL(triggered()), this, SLOT(saveAs()));

	mQuitAct = new QAction(tr("&Quit"), this);
	mQuitAct->setShortcut(tr("Ctrl+Q"));
	mQuitAct->setToolTip(tr("Exit the application"));
	connect(mQuitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

	// edit menu
	mViewModeGroup = new QActionGroup(this);
	mViewModeGroup->setExclusive(true);
	for (int i=0; i<VM_MAX; ++i)
	{
		QString name = "";
		switch (i)
		{
		case VM_IMAGE:
			name += "image";
			break;
		case VM_GMM_MASK:
			name += "GMM mask";
			break;
		case VM_NLINK_MASK:
			name += "NLink mask";
			break;
		case VM_TLINK_MASK:
			name += "TLink mask";
			break;
		}
		mViewModeActs[i] = new QAction(tr("View %1").arg(name), this);
		mViewModeActs[i]->setCheckable(true);
		mViewModeActs[i]->setToolTip(tr("Toggle view of %1").arg(name));
		mViewModeGroup->addAction(mViewModeActs[i]);
	}
	mViewModeActs[VM_IMAGE]->setChecked(true);
	connect(mViewModeGroup, SIGNAL(triggered(QAction*)), this, SLOT(changeViewModeAct(QAction*)));

	mRefineAct = new QAction(tr("Refine"), this);
	mRefineAct->setToolTip(tr("Run GrabCut refinement"));

	mRefineOnceAct = new QAction(tr("Refine Once"), this);
	mRefineOnceAct->setToolTip(tr("Run one step of GrabCut refinement"));
	mRefineOnceAct->setShortcut(tr("Ctrl+R"));

	mFitGMMsAct = new QAction(tr("Fit GMMs"), this);
	mFitGMMsAct->setToolTip(tr("Run the Orchard-Bowman GMM clustering"));

	mAbortRefiningAct = new QAction(tr("Abort"), this);
	mAbortRefiningAct->setToolTip(tr("Stop refining that is running"));
	mAbortRefiningAct->setShortcut(tr("Ctrl+C"));

	mShowMaskAct = new QAction(tr("Show Mask"), this);
	mShowMaskAct->setToolTip(tr("Show alpha mask of the segmentation"));
	mShowMaskAct->setCheckable(true);
	mShowMaskAct->setChecked(mShowMask);
	mShowMaskAct->setShortcut(tr("Ctrl+M"));

	mEditActGroup = new QActionGroup(this);
	mEditActGroup->setExclusive(false);
	mEditActGroup->addAction(mRefineAct);
	mEditActGroup->addAction(mRefineOnceAct);
	mEditActGroup->addAction(mFitGMMsAct);
	mEditActGroup->addAction(mAbortRefiningAct);
	mEditActGroup->addAction(mShowMaskAct);
	connect(mEditActGroup, SIGNAL(triggered(QAction*)), this, SLOT(triggerEditAct(QAction*)));

	// view menu
	mViewToolBarAct = new QAction(tr("&Toolbar"), this);
	mViewToolBarAct->setToolTip(tr("Toggle toolbar"));
	mViewToolBarAct->setCheckable(true);
	mViewToolBarAct->setChecked(true);
	mViewToolBarAct->setShortcut(tr("Ctrl+T"));

	mInfoToolBarAct = new QAction(tr("&Information"), this);
	mInfoToolBarAct->setToolTip(tr("Toggle information toolbar"));
	mInfoToolBarAct->setCheckable(true);
	mInfoToolBarAct->setChecked(true);
	mInfoToolBarAct->setShortcut(tr("Ctrl+I"));

	mViewActGroup = new QActionGroup(this);
	mViewActGroup->addAction(mViewToolBarAct);
	mViewActGroup->addAction(mInfoToolBarAct);
	mViewActGroup->setExclusive(false);
	connect(mViewActGroup, SIGNAL(triggered(QAction*)), this, SLOT(triggerViewAct(QAction*)));

	// help menu
	mAboutAct = new QAction(tr("&About"), this);
	mAboutAct->setToolTip(tr("Show the application's About box"));
	connect(mAboutAct, SIGNAL(triggered()), this, SLOT(about()));
	mAboutQtAct = new QAction(tr("About &Qt"), this);
	mAboutQtAct->setToolTip(tr("Show the Qt library's About box"));
	connect(mAboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
}

void MainWindow::triggerEditAct(QAction* act)
{
	if (mImages[VM_IMAGE].isNull())
		return;

	if (act == mRefineOnceAct)
	{
		mGrabCut->refineOnce();
	}
	else if (act == mRefineAct)
	{
		mRefining = true;
	}
	else if (act == mFitGMMsAct)
	{
		mGrabCut->fitGMMs();
	}
	else if (act == mRefineAct)
	{
		mRefining = false;
	}
	else if (act == mShowMaskAct)
	{
		mShowMask = mShowMaskAct->isChecked();
	}
	updateImages();
	mImgView->update();
	updateInformationBar();
}

void MainWindow::triggerViewAct(QAction *act)
{
	bool bShow = act->isChecked();
	if (act == mViewToolBarAct)
	{
		mFileToolBar->setVisible(bShow);
		mEditToolBar->setVisible(bShow);
		mCameraLightToolBar->setVisible(bShow);
	}
	else if (act == mInfoToolBarAct)
	{
		mInfoToolBar->setVisible(bShow);
	}
}

void MainWindow::changeViewModeAct(QAction* act)
{
	for (int i=0; i<VM_MAX; ++i)
	{
		if (act == mViewModeActs[i])
		{
			mViewMode = static_cast<ViewMode>(i);
			mImgView->update();
			break;
		}
	}
}

void MainWindow::createMenus()
{
	mFileMenu = menuBar()->addMenu(tr("&File"));
	mFileMenu->addAction(mOpenAct);
	mFileMenu->addAction(mSaveAsImageAct);
	mFileMenu->addSeparator();
	mFileMenu->addAction(mQuitAct);
	
	menuBar()->addSeparator();

	mViewMenu = menuBar()->addMenu(tr("&View"));
	mViewMenu->addAction(mViewToolBarAct);
	mViewMenu->addAction(mInfoToolBarAct);

	menuBar()->addSeparator();

	mEditMenu = menuBar()->addMenu(tr("&Edit"));
	for (int i=0; i<VM_MAX; ++i)
	{
		mEditMenu->addAction(mViewModeActs[i]);
	}
	mEditMenu->addSeparator();
	mEditMenu->addAction(mRefineAct);
	mEditMenu->addAction(mRefineOnceAct);
	mEditMenu->addAction(mFitGMMsAct);
	mEditMenu->addAction(mAbortRefiningAct);
	mEditMenu->addSeparator();
	mEditMenu->addAction(mShowMaskAct);

	menuBar()->addSeparator();
	mHelpMenu = menuBar()->addMenu(tr("&Help"));
	mHelpMenu->addAction(mAboutAct);
	mHelpMenu->addAction(mAboutQtAct);
}

void MainWindow::createToolBars()
{
	// file toolbar
	mFileToolBar = addToolBar(tr("File"));
	mFileToolBar->addAction(mOpenAct);
	mFileToolBar->addAction(mSaveAsImageAct);

	// edit toolbar
	mEditToolBar = addToolBar(tr("Edit"));
	for (int i=0; i<VM_MAX; ++i)
	{
		mEditToolBar->addAction(mViewModeActs[i]);
	}

	// information toolbar
	mInfoToolBar = addToolBar(tr("Information"));
	addToolBar(Qt::RightToolBarArea, mInfoToolBar);

	QGroupBox *infoGroup = new QGroupBox(tr("Information"));
	//QProgressBar *progressBarCopy = new QProgressBar(mProgressBar);
	//connect(mProgressBar, SIGNAL(valueChanged(int)), progressBarCopy, SLOT(setValue(int)));
	QBoxLayout *infoLayout = new QBoxLayout(QBoxLayout::TopToBottom);
	mInfoLabel = new QLabel("");
	infoLayout->addWidget(mInfoLabel);
	//infoLayout->addWidget(progressBarCopy);
	infoGroup->setLayout(infoLayout);
	mInfoToolBar->addWidget(infoGroup);

	updateInformationBar();
}

void MainWindow::createStatusBar()
{
	statusBar()->addWidget(mProgressBar);
	mProgressBar->hide();
	
	statusBar()->showMessage(tr("Ready"));
	statusBar()->setSizeGripEnabled(false);
}

void MainWindow::updateImages()
{
	if (mShowMask)
		copy_alpha_array_to_QImage(mGrabCut->getAlphaImage(), mImgMask);

	copy_image_array_to_QImage(mGrabCut->getNLinksImage(), mImages[VM_NLINK_MASK]);
	copy_image_array_to_QImage(mGrabCut->getTLinksImage(), mImages[VM_TLINK_MASK]);
	copy_image_array_to_QImage(mGrabCut->getGMMsImage(), mImages[VM_GMM_MASK]);
}

void MainWindow::open()
{
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open Obj Model"), "objs", tr("Image Files (*.bmp *.png *.jpg)"));
	if (!fileName.isEmpty())
	{
		openImage(fileName);
		mImageArr.reset();
		mGrabCut.reset();
		mImageArr = std::auto_ptr<GrabCutNS::Image<GrabCutNS::Color> >(create_image_array_from_QImage(mImages[VM_IMAGE]));
		mGrabCut = std::auto_ptr<GrabCutNS::GrabCut>(new GrabCutNS::GrabCut(mImageArr.get()));

		initSystem();

		mImgView->update();

		setWindowTitle( tr("%1 - %2")
			.arg(strippedName(fileName))
			.arg(WINDOW_TITLE) );
	}
}

void MainWindow::saveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this, 
		tr("Save Result"), "res", tr("Images (*.png *.jpg *.bmp)"));
	
	saveAsImageFile(fileName);
}

void MainWindow::openImage(const QString& fileName)
{
	mImages[VM_IMAGE].load(fileName);
}

void MainWindow::saveAsImageFile(const QString& fileName)
{
	QImage img;
	switch (mViewMode)
	{
	case VM_IMAGE:
		img  = mImages[VM_IMAGE].convertToFormat(QImage::Format_ARGB32);
		copy_alpha_array_to_QImage(mGrabCut->getAlphaImage(), img, false);
		break;
	default:
		img = mImages[mViewMode];
		break;
	}
	img.save(fileName);
}

void MainWindow::about()
{
	QMessageBox::about(this, tr("About %1").arg(WINDOW_TITLE),
		tr("<p>A <b>Graph Cut</b> project writed by maxint, in July, 2010.</p>"
		"<p>Email: <a href='mailto:lnychina@gmail.com'>lnychina@gmail.com</a></p>"
		"<p>Blog: <a href='http://hi.baidu.com/maxint'>http://hi.baidu.com/maxint</a></p>"));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *e)
{
	if (obj->isWidgetType() &&
		obj->objectName() == WIDGET_NAME)
	{
		QWidget *wid = static_cast<QWidget*>(obj);
		if (e->type() == QEvent::Paint)
		{
			static QPen penDarkCyan(QBrush(Qt::darkCyan), 2);
			static QPen penBlue(QBrush(Qt::blue), 4);
			static QPen penRed(QBrush(Qt::red), 4);
			static QPainter painter;
			painter.begin(wid);
			QRect rect(QPoint(0,0), wid->size());
			painter.setBackground(QColor(200, 200, 200));
			painter.drawImage(rect, mImages[mViewMode]);
			if (mShowMask)
				painter.drawImage(rect, mImgMask);
			switch (mSelectionMode)
			{
			case SM_RECT:
				painter.setPen(penDarkCyan);
				painter.drawRect(QRect(mStartPos, mLastPos));
				break;
			case SM_PAINT_FG:
				painter.setPen(penRed);
				break;
			case SM_PAINT_BG:
				painter.setPen(penBlue);
				break;
			default:
				break;
			}
			if (mSelectionMode == SM_PAINT_FG || mSelectionMode == SM_PAINT_BG)
			{		
				painter.drawPoints(mPaintingPoses.data(), mPaintingPoses.size());
			}
			painter.end();
			e->accept();
			return true;
		}
		else if (mImages[VM_IMAGE].isNull())
		{
			// skip null image
		}
		else if (e->type() == QEvent::MouseButtonPress)
		{
			QMouseEvent *me = static_cast<QMouseEvent*>(e);
			if (mInitialized)
			{
				mPaintingPoses.clear();
				if (me->buttons() & Qt::LeftButton)
				{
					mSelectionMode = SM_PAINT_FG;
				}
				else
				{
					mSelectionMode = SM_PAINT_BG;
				}
				mPaintingPoses.append(me->pos());
			}
			else
			{
				if (me->buttons() & Qt::LeftButton)
				{
					mStartPos = mLastPos = me->pos();
					mSelectionMode = SM_RECT;
				}
			}
			mImgView->update();
		}
		else if (e->type() == QEvent::MouseMove)
		{
			QMouseEvent *me = static_cast<QMouseEvent*>(e);
			mLastPos = me->pos();
			if (mInitialized)
			{
				mPaintingPoses.append(mLastPos);
			}
			mImgView->update();
		}
		else if (e->type() == QEvent::MouseButtonRelease)
		{
			QMouseEvent *me = static_cast<QMouseEvent*>(e);
			mEndPos = me->pos();
			if (mInitialized)
			{
				mPaintingPoses.append(mEndPos);
				for (int i=0; i<mPaintingPoses.size(); ++i)
				{
					const QPoint &pt = mPaintingPoses.at(i);
					mGrabCut->setTrimap(pt.x()-2, pt.y()-2, pt.x()+2, pt.y()+2, 
						(mSelectionMode==SM_PAINT_FG) ? GrabCutNS::TrimapForeground : GrabCutNS::TrimapBackground);
				}
				mGrabCut->refineOnce();
				mGrabCut->buildImages();
				mSelectionMode = SM_NONE;
			}
			else
			{
				mGrabCut->initialize(mStartPos.x(), mStartPos.y(), mEndPos.x(), mEndPos.y());
				mGrabCut->fitGMMs();
				mSelectionMode = SM_NONE;
				mInitialized = true;
				mShowMask = true;
				mShowMaskAct->setChecked(mShowMask);
			}
			updateImages();
			mImgView->update();
		}
	}
	
	return QMainWindow::eventFilter(obj, e);
}

QString MainWindow::strippedName(const QString& fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

void MainWindow::updateInformationBar()
{
	QString info = tr("<style type='text/css'><!--"
		".info, { margin: 5px; }"
		"--></style>");
	info += tr("<table class='info'>");
	//info += tr("<tr><td>Primitives: </td><td>%1</td></tr>").arg(mEngine->getNumOfPrimitives());
	//info += tr("<tr><td>Resolution: </td><td>%1 x %2</td></tr>")
	//	.arg(mImage.width()).arg(mImage.height());
	//info += tr("<tr><td>Last cost time: </td><td>%1 ms</td></tr>").arg(mLastCostTime);
	//info += tr("<tr><td>Trace depth: </td><td>%1</td></tr>").arg(mEngine->getTraceDepth());
	//info += tr("<tr><td>Regular Samples: </td><td>%1</td></tr>").arg(mEngine->getRegularSampleSize());
	info += tr("</table>");
	mInfoLabel->setText(info);
}