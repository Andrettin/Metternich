import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: journal_view
	
	enum Mode {
		Finished,
		Active,
		Inactive
	}
	
	property int mode: JournalView.Mode.Active
	readonly property var country: metternich.game.player_country
	readonly property var country_game_data: country ? country.game_data : null
	property string status_text: ""
	property string middle_status_text: ""
	
	readonly property var journal_entries: journal_view.mode === JournalView.Mode.Finished ? country_game_data.finished_journal_entries : (journal_view.mode === JournalView.Mode.Active ? country_game_data.active_journal_entries : country_game_data.inactive_journal_entries)
	
	TiledBackground {
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: left_bar.right
		anchors.right: infopanel.left
		interface_style: "dark_wood_boards"
		frame_count: 8
	}
	
	ListView {
		id: journal_entry_list
		anchors.top: top_bar.bottom
		anchors.bottom: status_bar.top
		anchors.left: left_bar.right
		anchors.right: infopanel.left
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		model: journal_entries
		delegate: Item {
			width: journal_entry_list.width
			height: journal_entry_rectangle.height + entry_border.height
			
			readonly property var journal_entry: model.modelData
			
			readonly property string completion_conditions_string: journal_entry.get_completion_conditions_string()
			readonly property string completion_effects_string: journal_entry.get_completion_effects_string(metternich.game.player_country)
			readonly property string failure_conditions_string: journal_entry.get_failure_conditions_string()
			readonly property string failure_effects_string: journal_entry.get_failure_effects_string(metternich.game.player_country)
			
			Image {
				id: portrait
				anchors.top: parent.top
				anchors.left: parent.left
				source: "image://portrait/" + journal_entry.portrait.identifier
				fillMode: Image.Pad
			}
			
			Rectangle {
				id: portrait_border
				color: "gray"
				anchors.left: portrait.right
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: 1 * scale_factor
			}
			
			Rectangle {
				id: journal_entry_rectangle
				anchors.top: parent.top
				anchors.left: portrait_border.right
				anchors.right: parent.right
				height: portrait.height
				color: "transparent"
				clip: true
				
				SmallText {
					id: journal_entry_label
					text: journal_entry.name
					anchors.verticalCenter: parent.verticalCenter
					anchors.left: parent.left
					anchors.leftMargin: 8 * scale_factor
					anchors.right: completion_conditions_label.left
					anchors.rightMargin: 16 * scale_factor
					wrapMode: Text.WordWrap
				}
				
				SmallText {
					id: completion_conditions_label
					text: format_text(completion_conditions_string)
					anchors.verticalCenter: parent.verticalCenter
					anchors.right: completion_effects_label.left
					anchors.rightMargin: 16 * scale_factor
					width: 144 * scale_factor
					wrapMode: Text.WordWrap
					visible: completion_conditions_string.length > 0
				}
				
				SmallText {
					id: completion_effects_label
					text: format_text(completion_effects_string)
					anchors.verticalCenter: parent.verticalCenter
					anchors.right: failure_conditions_label.left
					anchors.rightMargin: 16 * scale_factor
					wrapMode: Text.WordWrap
					width: 144 * scale_factor
				}
				
				SmallText {
					id: failure_conditions_label
					text: format_text(failure_conditions_string)
					anchors.verticalCenter: parent.verticalCenter
					anchors.right: failure_effects_label.left
					anchors.rightMargin: 16 * scale_factor
					width: 144 * scale_factor
					wrapMode: Text.WordWrap
				}
				
				SmallText {
					id: failure_effects_label
					text: format_text(failure_effects_string)
					anchors.verticalCenter: parent.verticalCenter
					anchors.right: parent.right
					anchors.rightMargin: 8 * scale_factor
					wrapMode: Text.WordWrap
					width: 144 * scale_factor
				}
			}
			
			Rectangle {
				id: entry_border
				color: "gray"
				anchors.top: journal_entry_rectangle.bottom
				anchors.left: parent.left
				anchors.right: parent.right
				height: 1 * scale_factor
			}
		}
	}
	
	JournalInfoPanel {
		id: infopanel
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.right: parent.right
	}
	
	LeftBar {
		id: left_bar
		anchors.top: parent.top
		anchors.bottom: parent.bottom
		anchors.left: parent.left
	}
	
	StatusBar {
		id: status_bar
		anchors.bottom: parent.bottom
		anchors.left: left_bar.right
		anchors.right: infopanel.left
	}
	
	TopBar {
		id: top_bar
		anchors.top: parent.top
		anchors.left: left_bar.right
		anchors.right: infopanel.left
		prestige_visible: false
		
		SmallText {
			id: completion_conditions_top_label
			text: "Completion Conditions:"
			anchors.top: parent.top
			anchors.topMargin: 1 * scale_factor
			anchors.right: completion_effects_top_label.left
			anchors.rightMargin: 16 * scale_factor
			width: 144 * scale_factor
		}
		
		SmallText {
			id: completion_effects_top_label
			text: "Completion Effects:"
			anchors.top: parent.top
			anchors.topMargin: 1 * scale_factor
			anchors.right: failure_conditions_top_label.left
			anchors.rightMargin: 16 * scale_factor
			width: 144 * scale_factor
		}
		
		SmallText {
			id: failure_conditions_top_label
			text: "Failure Conditions:"
			anchors.top: parent.top
			anchors.topMargin: 1 * scale_factor
			anchors.right: failure_effects_top_label.left
			anchors.rightMargin: 16 * scale_factor
			width: 144 * scale_factor
		}
		
		SmallText {
			id: failure_effects_top_label
			text: "Failure Effects:"
			anchors.top: parent.top
			anchors.topMargin: 1 * scale_factor
			anchors.right: parent.right
			anchors.rightMargin: 8 * scale_factor
			width: 144 * scale_factor
		}
	}
}
