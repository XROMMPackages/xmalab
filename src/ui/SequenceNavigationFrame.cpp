/*
 * ProgressDialog.cpp
 *
 *  Created on: Nov 19, 2013
 *      Author: ben
 */


#include "ui/SequenceNavigationFrame.h"
#include "ui_SequenceNavigationFrame.h"

#include "ui/State.h"

SequenceNavigationFrame::SequenceNavigationFrame(QWidget *parent) :
												QFrame(parent),
												frame(new Ui::SequenceNavigationFrame){

	frame->setupUi(this);
	frame->spinBox->setMinimum(1);
	frame->horizontalSlider->setValue(0);
	frame->spinBox->setValue(1);
	connect(State::getInstance(), SIGNAL(activeFrameChanged(int)), this, SLOT(activeFrameChanged(int)));
}

SequenceNavigationFrame::~SequenceNavigationFrame(){
	delete frame;
}

void SequenceNavigationFrame::setNbImages(int nbImages){
	maxFrame = nbImages-1;
	frame->horizontalSlider->setMaximum(maxFrame);
	frame->spinBox->setMaximum(maxFrame+1);
	if(frame->horizontalSlider->value() == 0)frame->toolButtonPrev->setEnabled(false);
}

void SequenceNavigationFrame::activeFrameChanged(int activeFrame){
	if(activeFrame + 1 != frame->spinBox->value()){
		frame->spinBox->setValue(activeFrame + 1);
	}
	if(activeFrame != frame->horizontalSlider->value()){
		frame->horizontalSlider->setValue(activeFrame);
	}

	if(frame->spinBox->value() == frame->spinBox->maximum()){
		frame->toolButtonNext->setEnabled(false);
	}else{
		frame->toolButtonNext->setEnabled(true);
	}

	if(frame->spinBox->value() == frame->spinBox->minimum()){
		frame->toolButtonPrev->setEnabled(false);
	}else{
		frame->toolButtonPrev->setEnabled(true);
	}
}

void SequenceNavigationFrame::on_horizontalSlider_valueChanged(int value){
	State::getInstance()->changeActiveFrame(value);
}

void SequenceNavigationFrame::on_spinBox_valueChanged(int value){
	State::getInstance()->changeActiveFrame(value - 1);
}

void SequenceNavigationFrame::on_toolButtonNext_clicked(){
	State::getInstance()->changeActiveFrame(frame->horizontalSlider->value()+1);
}

void SequenceNavigationFrame::on_toolButtonPrev_clicked(){
	State::getInstance()->changeActiveFrame(frame->horizontalSlider->value()-1);
}
