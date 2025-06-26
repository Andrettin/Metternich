import QtQuick

TiledBackground {
	id: background_grid
	anchors.top: parent.top
	anchors.topMargin: -(parent.y % (32 * scale_factor))
	anchors.bottom: parent.bottom
	anchors.left: parent.left
	anchors.leftMargin: -(parent.x % (32 * scale_factor))
	anchors.right: parent.right
	rows: Math.ceil(parent.height / (32 * scale_factor)) + 1
	columns: Math.ceil(parent.width / (32 * scale_factor)) + 1
}
