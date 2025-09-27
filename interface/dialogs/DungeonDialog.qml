import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import ".."

DialogBase {
	id: dungeon_dialog
	height: button_column.y + button_column.height + 8 * scale_factor
	title: dungeon_name
	
	property var site: null
	property var dungeon: site ? site.game_data.dungeon : null
	readonly property string dungeon_name: dungeon ? (dungeon.random ? (dungeon.name + " of " + site.game_data.current_cultural_name) : dungeon.name) : ""
	readonly property bool can_visit_dungeon: site && dungeon && metternich.game.player_country.game_data.can_visit_site(site)
	
	PortraitButton {
		id: dungeon_portrait
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		portrait_identifier: dungeon ? dungeon.portrait.identifier : ""
	}
	
	SmallText {
		id: text_label
		anchors.top: dungeon_portrait.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: dungeon ? format_text(
			can_visit_dungeon ? ("Do you wish to explore " + (dungeon.random ? "the " : "") + dungeon_name + "?"
				+ (dungeon.level !== 0 ? ("\n\nDungeon Level: " + dungeon.level) : "")
			) : (dungeon.level !== 0 ? ("Dungeon Level: " + dungeon.level) : "")
		) : ""
		wrapMode: Text.WordWrap
		width: Math.min(text_label_proxy.contentWidth, parent.width - 16 * scale_factor)
		
		SmallText { //used to measure text, avoiding the binding loop of using the main text label's content width directly, given the wrap mode
			id: text_label_proxy
			anchors.horizontalCenter: parent.horizontalCenter
			text: text_label.text
			opacity: 0
		}
	}
	
	Column {
		id: button_column
		anchors.top: text_label.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 8 * scale_factor
		
		TextButton {
			id: yes_button
			text: "Yes"
			visible: can_visit_dungeon
			onClicked: {
				metternich.game.player_country.game_data.visit_target_site = site
				dungeon_dialog.close()
			}
		}
		
		TextButton {
			id: no_button
			text: "No"
			visible: can_visit_dungeon
			onClicked: {
				if (metternich.game.player_country.game_data.visit_target_site === site) {
					metternich.game.player_country.game_data.visit_target_site = null
				}
				dungeon_dialog.close()
			}
		}
		
		TextButton {
			id: ok_button
			text: "OK"
			visible: !can_visit_dungeon
			onClicked: {
				dungeon_dialog.close()
			}
		}
	}
	
	onClosed: {
		site = null
	}
}
