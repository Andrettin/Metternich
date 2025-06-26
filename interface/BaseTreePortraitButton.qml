import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal

Item {
	readonly property var tree_item: parent.parent
	readonly property var entry: model.modelData
	readonly property string name: entry.full_name ? entry.full_name : entry.name
	readonly property int horizontal_padding: 8 * scale_factor
	readonly property int vertical_padding: 32 * scale_factor
	readonly property int top_offset: -vertical_padding
	property int button_x: 0
	property int button_y: 0
	property int button_width: 1 //the width in buttons
	property bool has_tree_parent: false
	property int parent_button_x: 0
	property int parent_button_y: 0
	property int parent_button_width: 1
	readonly property int parent_x: parent_button_x * width + (parent_button_width - 1) * width / 2 + horizontal_padding
	readonly property int parent_y: parent_button_y * height + vertical_padding + top_offset
	property string parent_name: ""
	readonly property int parent_text_height: parent_text_height_reference.contentHeight + 2 * scale_factor * 2
	property bool tree_line_visible: true
	property bool grayscale: false
	property bool transparent: false
	property bool disabled: false
	property bool highlighted: false
	readonly property bool hovered: portrait_button.hovered
	
	Universal.accent: Universal.Taupe
	
	signal clicked()
	
	width: portrait_button.width + horizontal_padding * 2
	height: portrait_button.height + vertical_padding * 2
	x: button_x * width + (button_width - 1) * width / 2 + horizontal_padding
	y: button_y * height + vertical_padding + top_offset
	z: -button_y //make it so buttons farther below are on a lower Z level, so that their lines aren't drawn over upper buttons' text
	
	PortraitButton {
		id: portrait_button
		anchors.horizontalCenter: parent.horizontalCenter
		anchors.verticalCenter: parent.verticalCenter
		portrait_identifier: entry.portrait.identifier
		//grayscale: parent.grayscale
		transparent: parent.transparent
		//disabled: parent.disabled
		highlighted: parent.highlighted
		
		MouseArea {
			anchors.fill: parent
			hoverEnabled: true
			
			onEntered: {
				status_text = name
			}
			
			onExited: {
				status_text = ""
			}
			
			onReleased: {
				parent.parent.clicked()
			}
		}
	}
	
	TinyText {
		id: label
		anchors.top: portrait_button.bottom
		anchors.topMargin: 2 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		text: name
		width: parent.width
		wrapMode: Text.WordWrap
		horizontalAlignment: Text.AlignHCenter
	}
	
	TinyText {
		id: parent_text_height_reference
		opacity: 0 //make it 100% transparent, this is for calculating the parent text height only, it shouldn't be visible
		text: parent.parent_name
		width: parent.width
		wrapMode: Text.WordWrap
		horizontalAlignment: Text.AlignHCenter
	}
	
	Rectangle {
		id: parent_line
		width: 2 * scale_factor
		height: vertical_padding - parent_text_height
		color: "gray"
		x: parent.parent_x - parent.x + parent.width / 2 - (width / 2)
		y: parent.parent_y - parent.y + parent.height - vertical_padding + parent_text_height
		visible: parent.has_tree_parent && parent.tree_line_visible
	}
	
	Rectangle {
		id: child_line
		anchors.top: parent_line.bottom
		anchors.topMargin: parent.x == parent.parent_x && parent_line.height < 0 ? -parent_line.height : 0
		anchors.bottom: portrait_button.top
		width: 2 * scale_factor
		color: "gray"
		x: parent.width / 2 - (width / 2)
		visible: parent.has_tree_parent && parent.tree_line_visible
	}
	
	Rectangle {
		id: horizontal_line
		anchors.top: parent_line.bottom
		width: get_base_width() + 2 * scale_factor
		height: 2 * scale_factor
		color: "gray"
		x: parent.width / 2 - (child_line.width / 2) - (parent.x > parent.parent_x ? get_base_width() : 0)
		visible: parent.has_tree_parent && parent.x != parent.parent_x && parent.tree_line_visible
		
		function get_base_width() {
			var base_width = 0
			
			if (parent.x < parent.parent_x) {
				base_width = parent.parent_x - parent.x
			} else if (parent.x > parent.parent_x) {
				base_width = parent.x - parent.parent_x
			}
			
			if (parent_text_height > vertical_padding) {
				base_width -= parent_text_height_reference.contentWidth / 2
				base_width -= 2 * scale_factor
			}
			
			return base_width
		}
	}
	
	Item {
		anchors.top: parent_line.top
		anchors.bottom: child_line.bottom
		anchors.horizontalCenter: child_line.horizontalCenter
		width: secondary_parents_icon_grid.width
		
		Grid {
			id: secondary_parents_icon_grid
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.verticalCenter: parent.verticalCenter
			columns: (button_y - parent_button_y === 1) ? entry.secondary_tree_parents.length : 1
			rowSpacing: 0
			columnSpacing: 0
			
			Repeater {
				model: entry.secondary_tree_parents
				
				IconButton {
					id: secondary_parent_icon_button
					icon_identifier: secondary_parent.icon.identifier
					highlighted: secondary_parent.class_name === "metternich::technology" && metternich.game.player_country.game_data.has_technology(secondary_parent)
					circle: true
					
					readonly property var secondary_parent: model.modelData
					
					MouseArea {
						anchors.fill: parent
						hoverEnabled: true
						
						onEntered: {
							status_text = "Prerequisite: " + secondary_parent.name
						}
						
						onExited: {
							//only clear the status text on exist if it was actually still the text set by this
							if (status_text === ("Prerequisite: " + secondary_parent.name)) {
								status_text = ""
							}
						}
					}
				}
			}
		}
	}
}
