FILE(GLOB XMALab_FORMS_LOCAL
    "*.ui"
)

SET(XMALab_FORMS
	${XMALab_FORMS}
	${XMALab_FORMS_LOCAL}
	PARENT_SCOPE
)