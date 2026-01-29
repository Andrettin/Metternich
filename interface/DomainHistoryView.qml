import QtQuick
import QtQuick.Controls
import "./dialogs"

Item {
	id: domain_history_view
	
	readonly property var domain: politics_view.country
	readonly property var domain_game_data: domain ? domain.game_data : null
	
	ListView {
		id: historical_rulers_list
		anchors.fill: parent
		boundsBehavior: Flickable.StopAtBounds
		clip: true
		model: domain_game_data.historical_rulers
		delegate: Item {
			width: historical_rulers_list.width
			height: historical_ruler_rectangle.height + entry_border.height
			
			readonly property date historical_ruler_date: model.modelData.key
			readonly property var historical_ruler: model.modelData.value
			
			Image {
				id: portrait
				anchors.top: parent.top
				anchors.left: parent.left
				source: "image://portrait/" + historical_ruler.game_data.portrait.identifier
				fillMode: Image.Pad
				
				MouseArea {
					anchors.fill: parent
					onClicked: {
						if (character_dialog.opened && character_dialog.character === historical_ruler) {
							return
						}
						
						character_dialog.character = historical_ruler
						character_dialog.open()
					}
				}
			}
			
			Rectangle {
				id: portrait_border
				color: "gray"
				anchors.left: portrait.right
				anchors.top: parent.top
				anchors.bottom: parent.bottom
				width: 1 * scale_factor
			}
			
			Item {
				id: historical_ruler_rectangle
				anchors.top: parent.top
				anchors.left: portrait_border.right
				anchors.right: parent.right
				height: portrait.height
				clip: true
				
				SmallText {
					id: historical_ruler_label
					text: metternich.game.year_to_labeled_qstring(date_year(historical_ruler_date)) + ": " + historical_ruler.game_data.get_full_name_for_domain(domain)
					anchors.verticalCenter: parent.verticalCenter
					anchors.left: parent.left
					anchors.leftMargin: 8 * scale_factor
					wrapMode: Text.WordWrap
				}
				
				SmallText {
					id: historical_ruler_dynasty_label
					text: historical_ruler.dynasty ? historical_ruler.dynasty.name : ""
					anchors.verticalCenter: parent.verticalCenter
					anchors.left: parent.right
					anchors.leftMargin: -128 * scale_factor
					wrapMode: Text.WordWrap
				}
			}
			
			Rectangle {
				id: entry_border
				color: "gray"
				anchors.top: portrait.bottom
				anchors.left: parent.left
				anchors.right: parent.right
				height: 1 * scale_factor
			}
		}
	}
}
