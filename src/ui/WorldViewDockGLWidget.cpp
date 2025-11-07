//  ----------------------------------
//  XMALab -- Copyright (c) 2015, Brown University, Providence, RI.
//
//  All Rights Reserved
//
//  Use of the XMALab software is provided under the terms of the GNU General Public License version 3
//  as published by the Free Software Foundation at http://www.gnu.org/licenses/gpl-3.0.html, provided
//  that this copyright notice appear in all copies and that the name of Brown University not be used in
//  advertising or publicity pertaining to the use or distribution of the software without specific written
//  prior permission from Brown University.
//
//  See license.txt for further information.
//
//  BROWN UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE WHICH IS
//  PROVIDED "AS IS", INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//  FOR ANY PARTICULAR PURPOSE.  IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE FOR ANY
//  SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR FOR ANY DAMAGES WHATSOEVER RESULTING
//  FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
//  OTHER TORTIOUS ACTION, OR ANY OTHER LEGAL THEORY, ARISING OUT OF OR IN CONNECTION
//  WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
//  ----------------------------------
//
///\file WorldViewDockGLWidget.cpp
///\author Benjamin Knorlein (original OpenGL implementation)
///\author GitHub Copilot (Qt Quick integration)

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "ui/WorldViewDockGLWidget.h"

#include <QColor>
#include <QQmlContext>
#include <QQuickWidget>
#include <QVBoxLayout>
#include <QtGlobal>

#include "ui/State.h"

namespace
{
constexpr const char* kWorldViewQmlUrl = "qrc:/qml/WorldViewScene.qml";
}

namespace xma
{
class WorldViewDockGLWidget::ViewModel : public QObject
{
	Q_OBJECT
	Q_PROPERTY(int frame READ frame WRITE setFrame NOTIFY frameChanged)
	Q_PROPERTY(bool useCustomTimeline READ useCustomTimeline WRITE setUseCustomTimeline NOTIFY useCustomTimelineChanged)
	Q_PROPERTY(double focalPlaneDistance READ focalPlaneDistance WRITE setFocalPlaneDistance NOTIFY focalPlaneDistanceChanged)
	Q_PROPERTY(int workspace READ workspace NOTIFY workspaceChanged)
public:
	explicit ViewModel(QObject* parent = nullptr)
		: QObject(parent), m_frame(0), m_useCustomTimeline(false), m_focalPlaneDistance(200.0)
	{
		connect(State::getInstance(), &State::workspaceChanged, this, [this](work_state) {
			emit workspaceChanged(workspace());
		});
	}

	int frame() const { return m_frame; }
	bool useCustomTimeline() const { return m_useCustomTimeline; }
	double focalPlaneDistance() const { return m_focalPlaneDistance; }
	int workspace() const { return static_cast<int>(State::getInstance()->getWorkspace()); }

	void setFrame(int value)
	{
		if (m_frame == value)
		{
			emit frameChanged(m_frame);
			return;
		}
		m_frame = value;
		emit frameChanged(m_frame);
	}

	void setUseCustomTimeline(bool value)
	{
		if (m_useCustomTimeline == value)
		{
			return;
		}
		m_useCustomTimeline = value;
		emit useCustomTimelineChanged(m_useCustomTimeline);
	}

	void setFocalPlaneDistance(double value)
	{
		if (qFuzzyCompare(1.0 + m_focalPlaneDistance, 1.0 + value))
		{
			return;
		}
		m_focalPlaneDistance = value;
		emit focalPlaneDistanceChanged(m_focalPlaneDistance);
	}

signals:
	void frameChanged(int frame);
	void useCustomTimelineChanged(bool useCustomTimeline);
	void focalPlaneDistanceChanged(double focalPlaneDistance);
	void workspaceChanged(int workspace);

private:
	int m_frame;
	bool m_useCustomTimeline;
	double m_focalPlaneDistance;
};
} // namespace xma

using namespace xma;

WorldViewDockGLWidget::WorldViewDockGLWidget(QWidget* parent)
	: QWidget(parent), m_viewModel(new ViewModel(this)), m_quickWidget(new QQuickWidget(this))
{
	setMinimumSize(50, 50);
	setFocusPolicy(Qt::StrongFocus);

	m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
	m_quickWidget->setClearColor(QColor(20, 20, 20));
	m_quickWidget->setFocusPolicy(Qt::NoFocus);
	m_quickWidget->rootContext()->setContextProperty(QStringLiteral("worldViewModel"), m_viewModel);

	initializeQuickScene();

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_quickWidget);
}

WorldViewDockGLWidget::~WorldViewDockGLWidget() = default;

void WorldViewDockGLWidget::initializeQuickScene()
{
	m_quickWidget->setSource(QUrl(QLatin1String(kWorldViewQmlUrl)));
}

void WorldViewDockGLWidget::setUseCustomTimeline(bool value)
{
	m_viewModel->setUseCustomTimeline(value);
}

void WorldViewDockGLWidget::setFrame(int value)
{
	m_viewModel->setFrame(value);
}

void WorldViewDockGLWidget::animate()
{
	// Emit the frameChanged signal even when the frame number is unchanged so QML can refresh state.
	m_viewModel->setFrame(m_viewModel->frame());
}

void WorldViewDockGLWidget::setFocalPlaneDistance(float distance)
{
	m_viewModel->setFocalPlaneDistance(distance);
}

#include "WorldViewDockGLWidget.moc"

