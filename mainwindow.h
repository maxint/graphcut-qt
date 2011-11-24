#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>
#include "GrabCut.h"
#include "Image.h"
#include "Color.h"

class QMenu;
class QLabel;
class QAction;
class QActionGroup;
class QDoubleSpinBox;
class QProgressBar;

//namespace GrabCutNS {
//	class GrabCut;
//}

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow();

protected:
	 bool eventFilter(QObject *obj, QEvent *e);
	 QSize sizeHint() const;

private:
	void setupUi();
	void createActions();
	void createMenus();
	void createToolBars();
	void createStatusBar();

	void init();
	void initSystem();
	void initParameters();

	void openImage(const QString& fileName);
	void saveAsImageFile(const QString& fileName);
	QString strippedName(const QString& fullFileName);
	void updateInformationBar();

	void updateImages();

private slots:
	void open();
	void saveAs();
	void triggerEditAct(QAction* act);
	void triggerViewAct(QAction* act);
	void changeViewModeAct(QAction* act);
	void about();

private:
	enum ViewMode
	{
		VM_IMAGE = 0,
		VM_GMM_MASK,
		VM_NLINK_MASK,
		VM_TLINK_MASK,
		VM_MAX
	};

	enum SelectionMode
	{
		SM_NONE = 0,
		SM_RECT,
		SM_PAINT_BG,
		SM_PAINT_FG
	};

private:
	QWidget *mImgView;
	QImage mImages[VM_MAX];
	QImage mImgMask;

	// file menu
	QMenu *mFileMenu;
	QAction *mOpenAct;
	QAction *mQuitAct;
	QAction *mSaveAsImageAct;

	// view menu
	QMenu *mViewMenu;
	QAction *mViewToolBarAct;
	QAction *mInfoToolBarAct;
	QActionGroup *mViewActGroup;

	// edit menu
	QMenu *mEditMenu;
	QAction *mViewModeActs[VM_MAX];
	QActionGroup *mViewModeGroup;
	QAction *mRefineAct;
	QAction *mRefineOnceAct;
	QAction *mFitGMMsAct;
	QAction *mAbortRefiningAct;
	QAction *mShowMaskAct;
	QActionGroup *mEditActGroup;

	// help menu
	QMenu *mHelpMenu;
	QAction *mAboutAct;
	QAction *mAboutQtAct;

	QToolBar *mFileToolBar;
	QToolBar *mEditToolBar;
	QToolBar *mCameraLightToolBar;

	// info toolbar
	QToolBar *mInfoToolBar;
	QLabel *mInfoLabel;
	long mLastCostTime;

	// status bar
	QLabel *mResLabel;
	QProgressBar *mProgressBar;

	// mouse operations
	QPoint mLastPos;
	QPoint mStartPos;
	QPoint mEndPos;

	// view mode
	ViewMode mViewMode;

	// grabcut
	std::auto_ptr<GrabCutNS::Image<GrabCutNS::Color> > mImageArr;
	std::auto_ptr<GrabCutNS::GrabCut> mGrabCut;
	QVector<QPoint> mPaintingPoses;
	SelectionMode mSelectionMode;
	bool mRefining;
	bool mInitialized;
	bool mShowMask;
};

#endif // MAINWINDOW_H
