#pragma once
#include "WickedEngine.h"

class EditorComponent;

class IKWindow : public wiWindow
{
public:
	IKWindow(EditorComponent* editor);

	wiECS::Entity entity;
	void SetEntity(wiECS::Entity entity);

	wiButton* createButton;
	wiComboBox* targetCombo;
	wiCheckBox* disabledCheckBox;
	wiSlider* chainLengthSlider;
	wiSlider* iterationCountSlider;
};

