import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

Map {
	id: geoprovince_map
	plugin: Plugin { name: "itemsoverlay" }	
	center: QtPositioning.coordinate(45, 10)
	zoomLevel: 8
	minimumZoomLevel: 7
	color: "black"
	onZoomLevelChanged: {
		if (geoprovince_map.zoomLevel < geoprovince_map.minimumZoomLevel) {
			geoprovince_map.zoomLevel = geoprovince_map.minimumZoomLevel
		}
	}
	
	WheelHandler {
		id: wheel
		// workaround for QTBUG-87646 / QTBUG-112394 / QTBUG-112432:
		// Magic Mouse pretends to be a trackpad but doesn't work with PinchHandler
		// and we don't yet distinguish mice and trackpads on Wayland either
		acceptedDevices: Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland"
						 ? PointerDevice.Mouse | PointerDevice.TouchPad
						 : PointerDevice.Mouse
		rotationScale: 1/120
		property: "zoomLevel"
	}

	DragHandler {
		id: drag
		target: null
		onTranslationChanged: (delta) => geoprovince_map.pan(-delta.x, -delta.y)
	}
	
	Repeater {
		model: metternich.map.provinces.length > 0 ? metternich.map.provinces[0].world.province_geoshapes : []
		
		MapPolygon {
			id: province_geopolygon
			color: province.game_data.owner ? province.game_data.owner.color : (province.water_zone ? metternich.defines.ocean_color : metternich.defines.minor_nation_color)
			path: geoshape.perimeter
			border.width: 1 * scale_factor
			
			readonly property var province: model.modelData.key
			readonly property var geoshape: model.modelData.value
		}
	}
	
	Repeater {
		model: metternich.map.sites
		
		MapQuickItem {
			id: site_item
			coordinate: site.qgeocoordinate
			anchorPoint.x: Math.floor(site_icon.width / 2)
			anchorPoint.y: Math.floor(site_icon.height / 2)
			visible: geoprovince_map.zoomLevel >= 8
			
			readonly property var site: model.modelData
			readonly property var holding_type: site.game_data.holding_type
			readonly property var dungeon: site.game_data.dungeon
			
			sourceItem: Image {
				id: site_icon
				source: "image://icon/" + (holding_type ? holding_type.icon.identifier : (dungeon ? dungeon.icon.identifier : "garrison")) + (site === selected_site ? "/selected" : "")
			}
		}
	}
}
