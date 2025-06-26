import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Universal
import QtQuick.Window
import "./menus"

ApplicationWindow {
	id: window
	visible: true
	title: qsTr("Iron Barons")
	width: Screen.width
	height: Screen.height + 1 //it needs to be +1 otherwise it becomes (non-borderless) fullscreen automatically
	flags: Qt.FramelessWindowHint | Qt.Window
	font.family: berenika_font.name
	font.pixelSize: 10 * scale_factor
	Universal.theme: Universal.Dark
	
	readonly property real scale_factor: metternich.scale_factor
	readonly property color interface_background_color: Qt.rgba(48.0 / 255.0, 48.0 / 255.0, 48.0 / 255.0, 1)
	
	property int politics_view_mode: PoliticsView.Advisors
	property int technology_view_mode: TechnologyView.Mode.Available
	property var technology_view_category: null
	property var technology_view_subcategory: null
	property int trade_view_mode: TradeView.TradeOrders
	
	property var open_dialogs: []
	
	readonly property var notification_dialog_component: Qt.createComponent("dialogs/NotificationDialog.qml")
	
	FontLoader {
		id: berenika_font
		source: "../fonts/berenika.ttf"
	}
	
	FontLoader {
		id: berenika_bold_font
		source: "../fonts/berenika_bold.ttf"
	}
	
	Item { //tooltips need to be attached to an item
		id: tooltip_manager
		
		property int tooltip_y_override: 0
	}
	
	MenuStack {
		id: menu_stack
		initialItem: "menus/MainMenu.qml"
		visible: metternich.running
	}
	
	Connections {
		target: metternich
		function onRunningChanged() {
			if (metternich.running) {
				//metternich.get_world("earth").write_province_image(0, 1)
				//metternich.get_map_template("earth").write_province_image()
				//metternich.get_map_template("earth").write_terrain_image()
				//metternich.get_map_template("earth").write_river_image()
				//metternich.get_map_template("earth").write_border_river_image()
				//metternich.get_map_template("earth").write_route_image()
			}
		}
	}
	
	Connections {
		target: metternich.game
		function onRunningChanged() {
			if (metternich.game.running) {
				//replace the scenario menu or the random map menu with the map view
				menu_stack.replace("CustomMapView.qml")
			}
		}
	}
	
	function colored_text(text, color) {
		var font_color_str = "<font color=\"" + color + "\">"
		
		text = text.replace(/<font color=\"gold\">/g, font_color_str)
		text = text.replace(/(?:<font color=\"#)(.{6})(?:\">)/g, font_color_str)
		return font_color_str + text + "</font>"
	}
	
	function highlight(text) {
		//highlight text
		return colored_text(text, "gold")
	}
	
	function format_text(text) {
		var str = text
		str = str.replace(/\n/g, "<br>")
		str = str.replace(/\t/g, "<font color=\"transparent\">aaaa</font>") //whitespaces are ignored after a <br>
		str = str.replace(/~</g, "<font color=\"gold\">")
		str = str.replace(/~>/g, "</font>")
		return str
	}
	
	function font_size_text(text, font_size) {
		return "<span style='font-size: " + font_size + "px;'>" + text + "</span>"
	}
	
	function small_text(text) {
		return font_size_text(text, 10 * scale_factor)
	}
	
	//generate a random number
	function random(n) {
		return Math.floor(Math.random() * n)
	}
	
	//format a number as text
	function number_string(n) {
		return n.toLocaleString(Qt.locale("en_US"), 'f', 0)
	}
	
	function roman_number_string(n) {
		const dict = {
			M: 1000,
			CM:900,
			D:500,
			CD:400,
			C:100,
			XC:90,
			L:50,
			XL:40,
			X:10,
			IX:9,
			V:5,
			IV:4,
			I:1
		}
		
		var str = ""
		
		for (var key in dict) {
			while (n >= dict[key]) {
				str += key
				n -= dict[key]
			}
		}
		
		return str
	}
	
	function signed_number_string(n) {
		if (n < 0) {
			return 0
		}
		return "+" + n
	}
	
	function date_year(date) {
		var year = date.getUTCFullYear()
		
		if (year < 0) {
			year -= 1 //-1 is needed, as otherwise negative dates are off by one
		}
		
		return year
	}
	
	function date_year_string(date) {
		var year = date_year(date)
		return year_string(year)
	}
	
	function year_string(year) {
		var year_suffix = ""
		
		if (year < 0) {
			year_suffix = " BC"
			year = Math.abs(year)
		}
		
		var year_str
		if (year >= 10000) {
			year_str = number_string(year)
		} else {
			year_str = year
		}
		
		return year_str + year_suffix
	}
	
	function date_year_range_string(date1, date2) {
		return year_range_string(date_year(date1), date_year(date2))
	}
	
	function year_range_string(year1, year2) {
		return Math.abs(year1) + "-" + year_string(year2)
	}
	
	function color_hex_string(color) {
		var red_hex_str = Math.floor(color.r * 255).toString(16)
		if (red_hex_str.length < 2) {
			red_hex_str = "0" + red_hex_str
		}
		
		var green_hex_str = Math.floor(color.g * 255).toString(16)
		if (green_hex_str.length < 2) {
			green_hex_str = "0" + green_hex_str
		}
		
		var blue_hex_str = Math.floor(color.b * 255).toString(16)
		if (blue_hex_str.length < 2) {
			blue_hex_str = "0" + blue_hex_str
		}
		
		return red_hex_str + green_hex_str + blue_hex_str
	}
	
	function string_list_to_string(str_list, separator, ignore_empty_strings = false) {
		var str = ""
		var first = true
		
		for (var i = 0; i < str_list.length; i++) {
			if (ignore_empty_strings && str_list[i].length == 0) {
				continue
			}
			
			if (first) {
				first = false
			} else {
				str += separator
			}
			
			str += str_list[i]
		}
		
		return str
	}
	
	function object_list_to_name_list(object_list, prefix = "", suffix = "") {
		var name_list = []
		
		for (var object of object_list) {
			name_list.push(prefix + object.name + suffix)
		}
		
		return name_list
	}
	
	function object_list_to_full_name_list(object_list, prefix = "", suffix = "") {
		var name_list = []
		
		for (var object of object_list) {
			name_list.push(prefix + object.full_name + suffix)
		}
		
		return name_list
	}
	
	function costs_to_string(costs, modifier) {
		var str = "Costs:"
		
		for (var i = 0; i < costs.length; i++) {
			var commodity = costs[i].key
			var cost = costs[i].value
			
			if (modifier) {
				cost = Math.floor(cost * modifier / 100)
			}
			
			str += "\n\t" + cost + " " + highlight(commodity.name)
		}
		
		return str
	}
	
	function get_plural_form(str) {
		if (str.endsWith("y")) {
			return str.substr(0, str.length - 1) + "ies"
		}
		
		return str + "s"
	}
	
	function add_notification(title, portrait_object, text, notification_parent) {
		if (notification_dialog_component.status == Component.Error) {
			console.error(notification_dialog_component.errorString())
			return
		}
		
		var dialog = notification_dialog_component.createObject(notification_parent, {
			title: title,
			portrait_object: portrait_object,
			text: text
		})
		
		dialog.open()
	}
}
