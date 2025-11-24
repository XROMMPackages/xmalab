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
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

#include "ui/WorldViewDockGLWidget.h"

#include <QColor>
#include <QQmlContext>
#include <QQuickView>
#include <QSizePolicy>
#include <QUrl>
#include <QVBoxLayout>

#include "ui/quick3d/Quick3DRendererControl.h"
#include "ui/quick3d/WorldViewSceneBridge.h"
namespace
{
constexpr const char* kWorldViewQmlUrl = "qrc:/qml/WorldViewScene.qml";
}

using namespace xma;


WorldViewDockGLWidget::WorldViewDockGLWidget(QWidget* parent)
	: QWidget(parent), m_viewModel(new WorldViewSceneBridge(this)), m_quickView(new QQuickView()), m_quickContainer(nullptr)
{
	setMinimumSize(50, 50);
	setFocusPolicy(Qt::StrongFocus);

	m_quickView->setResizeMode(QQuickView::SizeRootObjectToView);
	m_quickView->setColor(QColor(20, 20, 20));
#ifdef XMA_ENABLE_QRHI_RENDERING
	xma::quick3d::RendererControl::configureSurface(m_quickView);
#endif
	m_quickView->rootContext()->setContextProperty(QStringLiteral("worldViewModel"), m_viewModel);

	m_quickContainer = QWidget::createWindowContainer(m_quickView, this);
	m_quickContainer->setFocusPolicy(Qt::NoFocus);
	m_quickContainer->setMinimumSize(50, 50);
	m_quickContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	initializeQuickScene();

	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_quickContainer);
}

WorldViewDockGLWidget::~WorldViewDockGLWidget()
{
	delete m_quickView;
	m_quickView = nullptr;
	m_quickContainer = nullptr;
}

void WorldViewDockGLWidget::initializeQuickScene()
{
	m_quickView->setSource(QUrl(QLatin1String(kWorldViewQmlUrl)));
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

