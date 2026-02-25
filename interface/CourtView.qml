import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: advisors_view
	
	readonly property var minister_offices: get_minister_offices(country_game_data.government.available_offices)
	readonly property var offices: filter_offices(country_game_data.government.available_offices)
	readonly property var characters: country_game_data.characters
	readonly property var heir: country_game_data.government.heir
	
	PortraitGridItem {
		id: ruler_portrait
		anchors.top: parent.top
		anchors.topMargin: minister_portrait_grid.spacing
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: ruler ? ruler.game_data.portrait.identifier : ""
		visible: ruler !== null
		
		onClicked: {
			character_dialog.office = null
			character_dialog.character = ruler
			character_dialog.open()
		}
		
		onEntered: {
			status_text = ruler.game_data.titled_name
		}
		
		onExited: {
			status_text = ""
		}
	}
	
	PortraitGridItem {
		id: heir_portrait
		anchors.top: ruler_portrait.visible ? ruler_portrait.bottom : parent.top
		anchors.topMargin: minister_portrait_grid.spacing
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: heir ? heir.game_data.portrait.identifier : ""
		visible: heir !== null
		
		onClicked: {
			character_dialog.office = null
			character_dialog.character = heir
			character_dialog.open()
		}
		
		onEntered: {
			status_text = heir.game_data.titled_name
		}
		
		onExited: {
			status_text = ""
		}
	}
	
	PortraitGrid {
		id: minister_portrait_grid
		anchors.top: heir_portrait.visible ? heir_portrait.bottom : (ruler_portrait.visible ? ruler_portrait.bottom : parent.top)
		anchors.topMargin: spacing
		anchors.horizontalCenter: parent.horizontalCenter
		entries: minister_offices
		delegate: PortraitGridItem {
			portrait_identifier: appointed_office_holder ? appointed_office_holder.game_data.portrait.identifier + "/grayscale" : (office_holder ? office_holder.game_data.portrait.identifier : "no_character")
			
			readonly property var office: model.modelData
			readonly property var office_holder: country_game_data.government.get_office_holder(office)
			readonly property var appointed_office_holder: country_game_data.government.appointed_office_holders.length > 0 ? country_game_data.government.get_appointed_office_holder(office) : null //the check here is for the sake of property binding
			
			onClicked: {
				if (office_holder !== null || appointed_office_holder !== null) {
					character_dialog.office = office
					character_dialog.character = appointed_office_holder ? appointed_office_holder : office_holder
					character_dialog.open()
				} else {
					office_holder_choice_dialog.office = office
					office_holder_choice_dialog.open()
				}
			}
			
			onEntered: {
				status_text = appointed_office_holder ? "Appointing " + appointed_office_holder.full_name + " as " + country_game_data.government.get_office_title_name_qstring(office) : (office_holder ? office_holder.game_data.titled_name : country_game_data.government.get_office_title_name_qstring(office))
			}
				
			onExited: {
				status_text = ""
			}
		}
	}
	
	PortraitGrid {
		id: office_holder_portrait_grid
		anchors.top: minister_portrait_grid.visible ? minister_portrait_grid.bottom : (heir_portrait.visible ? heir_portrait.bottom : (ruler_portrait.visible ? ruler_portrait.bottom : parent.top))
		anchors.topMargin: spacing
		anchors.horizontalCenter: parent.horizontalCenter
		entries: offices
		delegate: PortraitGridItem {
			portrait_identifier: appointed_office_holder ? appointed_office_holder.game_data.portrait.identifier + "/grayscale" : (office_holder ? office_holder.game_data.portrait.identifier : "no_character")
			
			readonly property var office: model.modelData
			readonly property var office_holder: country_game_data.government.get_office_holder(office)
			readonly property var appointed_office_holder: country_game_data.government.appointed_office_holders.length > 0 ? country_game_data.government.get_appointed_office_holder(office) : null //the check here is for the sake of property binding
			
			onClicked: {
				if (office_holder !== null || appointed_office_holder !== null) {
					character_dialog.office = office
					character_dialog.character = appointed_office_holder ? appointed_office_holder : office_holder
					character_dialog.open()
				} else {
					office_holder_choice_dialog.office = office
					office_holder_choice_dialog.open()
				}
			}
			
			onEntered: {
				status_text = appointed_office_holder ? "Appointing " + appointed_office_holder.full_name + " as " + country_game_data.get_office_title_name_qstring(office) : (office_holder ? office_holder.game_data.titled_name : country_game_data.get_office_title_name_qstring(office))
			}
			
			onExited: {
				status_text = ""
			}
		}
	}
	
	PortraitGrid {
		id: character_portrait_grid
		anchors.top: office_holder_portrait_grid.visible ? office_holder_portrait_grid.bottom : (minister_portrait_grid.visible ? minister_portrait_grid.bottom : (heir_portrait.visible ? heir_portrait.bottom : (ruler_portrait.visible ? ruler_portrait.bottom : parent.top)))
		anchors.topMargin: spacing
		anchors.horizontalCenter: parent.horizontalCenter
		entries: filter_characters(characters)
		delegate: PortraitGridItem {
			portrait_identifier: character.game_data.portrait.identifier
			
			readonly property var character: model.modelData
			
			onClicked: {
				character_dialog.office = null
				character_dialog.character = character
				character_dialog.open()
			}
			
			onEntered: {
				status_text = character.full_name
			}
			
			onExited: {
				status_text = ""
			}
		}
	}
	
	OfficeHolderChoiceDialog {
		id: office_holder_choice_dialog
	}
	
	function get_minister_offices(offices) {
		var result = []
		
		for (var office of offices) {
			if (!office.minister) {
				continue
			}
			
			result.push(office)
		}
		
		return result
	}
	
	function filter_offices(offices) {
		var result = []
		
		for (var office of offices) {
			if (office.ruler || office.heir || office.minister) {
				continue
			}
			
			result.push(office)
		}
		
		return result
	}
	
	function filter_characters(characters) {
		var result = []
		
		for (var character of characters) {
			if (character.game_data.office !== null) {
				continue
			}
			
			result.push(character)
		}
		
		return result
	}
}
