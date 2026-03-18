import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: advisors_view
	
	readonly property var minister_offices: get_minister_offices(country_game_data.government.available_offices)
	readonly property var offices: filter_offices(country_game_data.government.available_offices)
	readonly property var characters: country_game_data.characters
	readonly property var heir: country_game_data.government.heir
	
	CharacterPortraitGridItem {
		id: ruler_portrait
		anchors.top: parent.top
		anchors.topMargin: minister_portrait_grid.spacing
		anchors.horizontalCenter: parent.horizontalCenter
		character: ruler
		visible: ruler !== null
		
		onHoveredChanged: {
			if (hovered) {
				status_text = ruler.game_data.titled_name
			} else {
				status_text = ""
			}
		}
	}
	
	CharacterPortraitGridItem {
		id: heir_portrait
		anchors.top: ruler_portrait.visible ? ruler_portrait.bottom : parent.top
		anchors.topMargin: minister_portrait_grid.spacing
		anchors.horizontalCenter: parent.horizontalCenter
		character: heir
		visible: heir !== null
		
		onHoveredChanged: {
			if (hovered) {
				status_text = heir.game_data.titled_name
			} else {
				status_text = ""
			}
		}
	}
	
	PortraitGrid {
		id: minister_portrait_grid
		anchors.top: heir_portrait.visible ? heir_portrait.bottom : (ruler_portrait.visible ? ruler_portrait.bottom : parent.top)
		anchors.topMargin: spacing
		anchors.horizontalCenter: parent.horizontalCenter
		entries: minister_offices
		delegate: CharacterPortraitGridItem {
			portrait_identifier: appointed_office_holder ? appointed_office_holder.game_data.portrait.identifier + "/grayscale" : (office_holder ? office_holder.game_data.portrait.identifier : "no_character")
			character: appointed_office_holder ? appointed_office_holder : office_holder
			
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
			
			onHoveredChanged: {
				if (hovered) {
					status_text = appointed_office_holder ? "Appointing " + appointed_office_holder.full_name + " as " + country_game_data.government.get_office_title_name_qstring(office) : (office_holder ? office_holder.game_data.titled_name : country_game_data.government.get_office_title_name_qstring(office))
				} else {
					status_text = ""
				}
			}
		}
	}
	
	PortraitGrid {
		id: office_holder_portrait_grid
		anchors.top: minister_portrait_grid.visible ? minister_portrait_grid.bottom : (heir_portrait.visible ? heir_portrait.bottom : (ruler_portrait.visible ? ruler_portrait.bottom : parent.top))
		anchors.topMargin: spacing
		anchors.horizontalCenter: parent.horizontalCenter
		entries: offices
		delegate: CharacterPortraitGridItem {
			portrait_identifier: appointed_office_holder ? appointed_office_holder.game_data.portrait.identifier + "/grayscale" : (office_holder ? office_holder.game_data.portrait.identifier : "no_character")
			character: appointed_office_holder ? appointed_office_holder : office_holder
			
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
			
			onHoveredChanged: {
				if (hovered) {
					status_text = appointed_office_holder ? "Appointing " + appointed_office_holder.full_name + " as " + country_game_data.get_office_title_name_qstring(office) : (office_holder ? office_holder.game_data.titled_name : country_game_data.get_office_title_name_qstring(office))
				} else {
					status_text = ""
				}
			}
		}
	}
	
	PortraitGrid {
		id: character_portrait_grid
		anchors.top: office_holder_portrait_grid.visible ? office_holder_portrait_grid.bottom : (minister_portrait_grid.visible ? minister_portrait_grid.bottom : (heir_portrait.visible ? heir_portrait.bottom : (ruler_portrait.visible ? ruler_portrait.bottom : parent.top)))
		anchors.topMargin: spacing
		anchors.horizontalCenter: parent.horizontalCenter
		entries: filter_characters(characters)
		delegate: CharacterPortraitGridItem {
			character: model.modelData
			
			onHoveredChanged: {
				if (hovered) {
					status_text = character.game_data.full_name
				} else {
					if (status_text === character.game_data.full_name) {
						status_text = ""
					}
				}
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
