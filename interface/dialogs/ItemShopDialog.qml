import QtQuick
import QtQuick.Controls
import ".."

DialogBase {
	id: item_shop_dialog
	title: "Buy Items"
	width: Math.max(content_column.width + 8 * scale_factor * 2, 256 * scale_factor)
	height: content_column.y + content_column.height + 8 * scale_factor
	
	property var item_slots: []
	
	Column {
		id: content_column
		anchors.top: title_item.bottom
		anchors.topMargin: 16 * scale_factor
		anchors.horizontalCenter: parent.horizontalCenter
		spacing: 16 * scale_factor
		
		ItemShopGrid {
			id: item_shop_grid
			item_slots: item_shop_dialog.item_slots
			label_visible: false
		}
		
		TextButton {
			id: ok_button
			anchors.horizontalCenter: parent.horizontalCenter
			text: "OK"
			onClicked: {
				item_shop_dialog.close()
			}
		}
	}
}
