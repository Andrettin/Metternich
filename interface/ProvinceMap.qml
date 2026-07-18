import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Flickable {
	id: province_map
	contentWidth: metternich.map.province_map_image_size.width * scale_factor
	contentHeight: metternich.map.province_map_image_size.height * scale_factor
	boundsBehavior: Flickable.StopAtBounds
	clip: true
	
	enum Mode {
		Political,
		Terrain,
		Cultural,
		Religious,
		Technology,
		TradeZone,
		Temple
	}
	
	enum SiteMode {
		Show,
		ShowLocations,
		Hide
	}
	
	property int mode: ProvinceMap.Mode.Political
	property int show_site_mode: ProvinceMap.SiteMode.Show
	readonly property var reference_country: selected_province ? selected_province.game_data.owner : (metternich.game.player_country ? metternich.game.player_country : null)
	property var hovered_site: null
	
	Item {
		id: unscaled_province_map
		width: metternich.map.province_map_image_size.width
		height: metternich.map.province_map_image_size.height
		layer.enabled: true
		
		Repeater {
			model: metternich.map.provinces
			
			/*
			Shape {
				id: province_shape
				visible: province_polygon_path !== null
				
				readonly property var province: model.modelData
				readonly property var province_polygon_path: province.map_data.polygon_paths.length > 0 ? province.map_data.polygon_paths[0] : null
				readonly property var province_polygon_rect: province.map_data.polygon_rects.length > 0 ? province.map_data.polygon_rects[0] : null
				
				ShapePath {
					strokeWidth: 1 * scale_factor
					strokeColor: "black"
					fillColor: selected_province === province ? metternich.defines.selected_country_color : province.game_data.map_color
					startX: 0
					startY: 0
					
					PathSvg {
						path: province_polygon_path ? province_polygon_path : ""
					}
				}
			}
			*/
			
			Image {
				id: province_image
				x: province ? province.game_data.map_image_rect.x : 0
				y: province ? province.game_data.map_image_rect.y : 0
				source: province ? ("image://province_map/" + province.identifier + (selected ? "/selected" : (interactive ? "/interactive" : get_map_mode_suffix(province_map.mode, province))) + "/" + change_count) : "image://empty/"
				cache: false
				
				readonly property var province: model.modelData
				readonly property var selected: selected_province === province && (selected_garrison === false || province_map.show_site_mode === ProvinceMap.SiteMode.ShowLocations)
				readonly property var interactive: selected_civilian_unit !== null && !selected_civilian_unit.busy && selected_civilian_unit_interactive_provinces.includes(province)
				property int change_count: 0
				
				Connections {
					target: province ? province.game_data : null
					
					function onMap_image_changed() {
						change_count += 1
					}
					
					function onMap_mode_image_changed(map_mode_identifier) {
						switch (mode) {
							case ProvinceMap.Mode.Political:
								return
							case ProvinceMap.Mode.Terrain:
								if (map_mode_identifier !== "terrain") {
									return
								}
								break
							case ProvinceMap.Mode.Cultural:
								if (map_mode_identifier !== "cultural") {
									return
								}
								break
							case ProvinceMap.Mode.Religious:
								if (map_mode_identifier !== "religious") {
									return
								}
								break
							case ProvinceMap.Mode.Technology:
								if (map_mode_identifier !== "technology") {
									return
								}
								break
							case ProvinceMap.Mode.TradeZone:
								if (map_mode_identifier !== "trade_zone") {
									return
								}
								break
							case ProvinceMap.Mode.Temple:
								if (map_mode_identifier !== "temple") {
									return
								}
								break
						}
						
						change_count += 1
					}
				}
			}
		}
	}
	
	ScaledImage {
		id: scaled_province_map
		width: unscaled_province_map.width * scale_factor
		height: unscaled_province_map.height * scale_factor
        source: unscaled_province_map
	}
	
	MouseArea {
		width: province_map.contentWidth
		height: province_map.contentHeight
		hoverEnabled: true
		
		onReleased: function (mouse) {
			metternich.defines.click_sound.play()
			
			var province = metternich.map.get_tile_province(Qt.point(Math.floor(mouse.x / metternich.defines.province_map_tile_scale / scale_factor), Math.floor(mouse.y / metternich.defines.province_map_tile_scale / scale_factor)))
			
			if (province === null || (selected_province === province && selected_garrison === false) || (province.water_zone && metternich.selected_military_units.length === 0)) {
				select_province(null)
			} else {
				if (metternich.selected_military_units.length > 0) {
					metternich.move_selected_military_units_to(province.game_data.provincial_capital ? province.game_data.provincial_capital.map_data.tile_pos : province.game_data.center_tile_pos)
					select_province(null)
					metternich.clear_selected_military_units()
					return
				} else if (selected_civilian_unit !== null && !selected_civilian_unit.moving && !selected_civilian_unit.working && selected_civilian_unit.can_move_to(province)) {
					selected_civilian_unit.move_to(province)
					select_civilian_unit(null)
					return
				}
				
				select_province(province)
			}
		}
		
		onPositionChanged: function (mouse) {
			var province = metternich.map.get_tile_province(Qt.point(Math.floor(mouse.x / metternich.defines.province_map_tile_scale / scale_factor), Math.floor(mouse.y / metternich.defines.province_map_tile_scale / scale_factor)))
			
			if (province !== null) {
				var text = get_province_status_text(province)
				status_text = text
			} else {
				status_text = ""
			}
		}
	}
	
	Repeater {
		model: metternich.game.active_routes
		
		Shape {
			id: route_shape
			visible: route.game_data.active && route.output_commodity !== null && (province_map.mode === ProvinceMap.Mode.TradeZone || route.output_commodity !== metternich.defines.wealth_commodity) && (province_map.mode === ProvinceMap.Mode.Temple || route.output_commodity !== metternich.defines.piety_commodity)
			
			readonly property var route: model.modelData
			readonly property var route_line_path: route.game_data.get_line_path()
			
			ShapePath {
				strokeWidth: 1 * scale_factor
				strokeColor: metternich.defines.selected_country_color
				fillColor: "transparent"
				capStyle: ShapePath.RoundCap
				joinStyle: ShapePath.RoundJoin
				startX: 0
				startY: 0
				
				PathSvg {
					path: route_line_path
				}
			}
		}
	}
	
	Repeater {
		model: metternich.map.provinces
		
		Item {
			id: province_label_area
			x: Math.max(Math.floor(text_rect.x * metternich.defines.province_map_tile_scale * scale_factor) + Math.floor((text_rect_width - width) / 2), Math.floor((province_label.contentWidth - width) / 2 + 1 * scale_factor))
			y: Math.floor(text_rect.y * metternich.defines.province_map_tile_scale * scale_factor) + Math.max(0, Math.floor((text_rect_height - height) / 2))
			width: province_label_column.width
			height: province_label_column.height
			
			readonly property var province: model.modelData
			readonly property var text_rect: province ? province.game_data.text_rect : Qt.rect(0, 0, 0, 0)
			readonly property int text_rect_width: Math.floor(text_rect.width * metternich.defines.province_map_tile_scale * scale_factor)
			readonly property int text_rect_height: Math.floor(text_rect.height * metternich.defines.province_map_tile_scale * scale_factor)
			
			Column {
				id: province_label_column
				spacing: 2 * scale_factor
				visible: province_map.show_site_mode !== ProvinceMap.SiteMode.ShowLocations
				
				Row {
					anchors.horizontalCenter: province_label_column.horizontalCenter
					spacing: 2 * scale_factor
					
					Image {
						id: garrison_icon
						source: "image://icon/" + (visible && province.water_zone ? 
							((province.game_data.military_unit_category_counts.length > 0 && province.game_data.get_domain_military_unit_category_counts(metternich.game.player_country).length > 0) ?
								province.game_data.get_domain_military_unit_icon(metternich.game.player_country).identifier
								: province.game_data.get_military_unit_icon().identifier
							) : "garrison")
							+ (selected ? "/selected" : "")
						visible: province && province.game_data.military_unit_category_counts.length > 0
						
						readonly property bool selected: visible && selected_province === province && selected_garrison
						
						MouseArea {
							anchors.fill: parent
							hoverEnabled: true
							
							onClicked: {
								metternich.defines.click_sound.play()
								selected_civilian_unit = null
								selected_site = null
								if (selected_province === province && selected_garrison) {
									selected_province = null
									selected_garrison = false
								} else {
									selected_province = province
									selected_garrison = true
								}
							}
							
							onContainsMouseChanged: {
								var text = province.water_zone ? "View Fleet" : "View Garrison"
								
								if (containsMouse) {
									status_text = text
								} else {
									if (status_text === text) {
										status_text = ""
									}
								}
							}
						}
					}
					
					Repeater {
						model: province ? province.game_data.civilian_units : []
						
						Image {
							id: civilian_unit_icon
							source: "image://icon/alliance" + (grayscale ? "/grayscale" : "") + (selected ? "/selected" : "")
							visible: civilian_unit.owner === metternich.game.player_country
							
							readonly property var civilian_unit: model.modelData
							readonly property bool civilian_unit_interactive: civilian_unit.owner === metternich.game.player_country
							readonly property bool selected: civilian_unit === selected_civilian_unit
							readonly property bool grayscale: civilian_unit.moving || civilian_unit.working
							
							MouseArea {
								anchors.fill: parent
								hoverEnabled: true
								
								onClicked: {
									metternich.defines.click_sound.play()
									if (civilian_unit_interactive && civilian_unit !== selected_civilian_unit) {
										select_civilian_unit(civilian_unit)
									} else {
										select_civilian_unit(null)
									}
								}
								
								onContainsMouseChanged: {
									update_civilian_unit_status_text(civilian_unit, containsMouse)
								}
							}
						}
					}
					
					Repeater {
						model: province ? province.game_data.entering_armies : []
						
						Image {
							id: entering_army_icon
							source: "image://icon/" + (province.water_zone && army.military_units.length > 0 ? army.get_military_unit_icon().identifier : "war") //FIXME: show the alliance icon if the army is entering a friendly province
							visible: army.military_units.length > 0
							
							readonly property var army: model.modelData
							
							Image {
								id: moving_fleet_icon
								anchors.top: parent.top
								anchors.topMargin: 8 * scale_factor
								anchors.right: parent.right
								anchors.rightMargin: 8 * scale_factor
								source: "image://icon/war" //FIXME: should be a steering wheel instead
							}
							
							MouseArea {
								anchors.fill: parent
								hoverEnabled: true
								
								onContainsMouseChanged: {
									var text = province.water_zone ? "Entering Fleet" : "Entering Army"
									
									if (containsMouse) {
										status_text = text
									} else {
										if (status_text === text) {
											status_text = ""
										}
									}
								}
							}
						}
					}
				}
				
				TinyText {
					id: province_label
					anchors.horizontalCenter: province_label_column.horizontalCenter
					text: province ? province.game_data.current_cultural_name : ""
					wrapMode: Text.WordWrap
					horizontalAlignment: Text.AlignHCenter
					verticalAlignment: Text.AlignVCenter
					width: province_label_column.width > 0 ? province_label_column.width : contentWidth
				}
				
				Grid {
					anchors.horizontalCenter: province_label_column.horizontalCenter
					spacing: 1 * scale_factor
					columns: 3
					visible: province_map.show_site_mode === ProvinceMap.SiteMode.Show
					
					Repeater {
						model: province ? province.game_data.visible_sites : []
						
						Item {
							id: site_icon_area
							width: site_icon.width + 4 * scale_factor
							height: site_icon.height + 4 * scale_factor
							
							readonly property var site: model.modelData
							readonly property var holding_type: site ? site.game_data.holding_type : null
							readonly property var dungeon: site ? site.game_data.dungeon : null
							readonly property bool selected: site === selected_site
							
							Rectangle {
								id: site_domain_color_circle
								width: site_icon_area.width
								height: site_icon_area.height
								radius: width / 2
								color: selected ? metternich.defines.selected_country_color : (site.game_data.owner ? site.game_data.owner.color : "transparent")
								visible: selected || (site.game_data.owner !== null && site.game_data.owner !== site.game_data.province.game_data.owner)
							}
							
							Image {
								id: site_icon
								anchors.verticalCenter: parent.verticalCenter
								anchors.horizontalCenter: parent.horizontalCenter
								source: "image://icon/" + (holding_type ? holding_type.icon.identifier : (dungeon ? dungeon.icon.identifier : (site.holding_type ? (site.holding_type.icon.identifier + "/blank_silhouette") : "garrison")))
							}
							
							MouseArea {
								anchors.fill: parent
								hoverEnabled: true
								
								onClicked: {
									metternich.defines.click_sound.play()
									selected_civilian_unit = null
									selected_province = null
									selected_garrison = false
									if (selected_site === site) {
										selected_site = null
									} else {
										selected_site = site
									}
								}
								
								onContainsMouseChanged: {
									var text = site.game_data.display_text
									
									if (containsMouse) {
										hovered_site = site
										status_text = text
									} else {
										if (status_text === text) {
											status_text = ""
										}
										
										if (hovered_site === site) {
											hovered_site = null
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	
	Repeater {
		model: metternich.map.sites
		
		Item {
			id: site_icon_area
			x: site ? Math.min(Math.max(site.game_data.tile_pos.x * metternich.defines.province_map_tile_scale * scale_factor - Math.floor(width / 2), 0), province_map.contentWidth - width) : 0
			y: site ? Math.min(Math.max(site.game_data.tile_pos.y * metternich.defines.province_map_tile_scale * scale_factor - Math.floor(height / 2), 0), province_map.contentHeight - height) : 0
			width: site_icon.width + 4 * scale_factor
			height: site_icon.height + 4 * scale_factor
			visible: province_map.show_site_mode === ProvinceMap.SiteMode.ShowLocations && (site.settlement || dungeon !== null)
			
			readonly property var site: model.modelData
			readonly property var tile_pos: site ? site.map_data.tile_pos : null
			readonly property var holding_type: site ? site.game_data.holding_type : null
			readonly property var dungeon: site ? site.game_data.dungeon : null
			readonly property bool selected: site === selected_site
			
			Rectangle {
				id: site_domain_color_circle
				width: site_icon_area.width
				height: site_icon_area.height
				radius: width / 2
				color: selected ? metternich.defines.selected_country_color : (site.game_data.owner ? site.game_data.owner.color : "transparent")
				visible: selected || (site.game_data.owner !== null && site.game_data.owner !== site.game_data.province.game_data.owner)
			}
			
			Image {
				id: site_icon
				anchors.verticalCenter: parent.verticalCenter
				anchors.horizontalCenter: parent.horizontalCenter
				source: "image://icon/" + (holding_type ? holding_type.icon.identifier : (dungeon ? dungeon.icon.identifier : (site.holding_type ? (site.holding_type.icon.identifier + "/blank_silhouette") : "garrison")))
			}
			
			MouseArea {
				anchors.fill: parent
				hoverEnabled: true
				
				onClicked: {
					metternich.defines.click_sound.play()
					selected_civilian_unit = null
					selected_province = null
					selected_garrison = false
					if (selected_site === site) {
						selected_site = null
					} else {
						selected_site = site
					}
				}
				
				onContainsMouseChanged: {
					var text = site.game_data.display_text
					
					if (containsMouse) {
						hovered_site = site
						status_text = text
					} else {
						if (status_text === text) {
							status_text = ""
						}
						
						if (hovered_site === site) {
							hovered_site = null
						}
					}
				}
			}
		}
	}
	
	function select_province(province) {
		selected_civilian_unit = null
		selected_site = null
		selected_province = province
		selected_garrison = false
	}
	
	function get_province_status_text(province) {
		var text = province.game_data.current_cultural_name
		
		if (province.game_data.owner !== null) {
			text += ", " + province.game_data.owner.game_data.name
			
			if (province.game_data.owner.game_data.realm !== province.game_data.owner) {
				text += ", " + province.game_data.owner.game_data.realm.game_data.name
			}
		}
		
		if (province_map.mode === ProvinceMap.Mode.Cultural && province.game_data.culture !== null) {
			text += " (" + province.game_data.culture.name + ")"
		} else if (province_map.mode === ProvinceMap.Mode.Religious && province.game_data.religion !== null) {
			text += " (" + province.game_data.religion.name + ")"
		} else if (province_map.mode === ProvinceMap.Mode.Terrain && province.map_data.terrain !== null) {
			text += " (" + province.map_data.terrain.name + ")"
		} else if (province_map.mode === ProvinceMap.Mode.Technology) {
			if (!province.water_zone) {
				text += " (" + province.game_data.technologies.length + " " + (province.game_data.technologies.length === 1 ? "Technology" : "Technologies") + ")"
			}
		} else if (province_map.mode === ProvinceMap.Mode.TradeZone) {
			var trade_zone_domain = province.game_data.trade_zone_domain
			if (trade_zone_domain !== null) {
				text += " (" + trade_zone_domain.game_data.name + ")"
			}
		} else if (province_map.mode === ProvinceMap.Mode.Temple) {
			var temple_domain = province.game_data.temple_domain
			if (temple_domain !== null) {
				text += " (" + temple_domain.game_data.name + ")"
			}
		}
		
		return text
	}
	
	function center_on_tile(tile_x, tile_y) {
		var pixel_x = Math.round(tile_x * metternich.defines.province_map_tile_scale * scale_factor - province_map.width / 2)
		var pixel_y = Math.round(tile_y * metternich.defines.province_map_tile_scale * scale_factor - province_map.height / 2)
		
		province_map.contentX = Math.min(Math.max(pixel_x, 0), province_map.contentWidth - province_map.width)
		province_map.contentY = Math.min(Math.max(pixel_y, 0), province_map.contentHeight - province_map.height)
	}
	
	function center_on_province(province) {
		center_on_tile(province.game_data.center_tile_pos.x, province.game_data.center_tile_pos.y)
	}
	
	function center_on_country_capital(country) {
		var capital = country.game_data.capital
		
		if (capital === null) {
			return
		}
		
		var capital_game_data = capital.game_data
		var capital_x = capital_game_data.tile_pos.x
		var capital_y = capital_game_data.tile_pos.y
		
		center_on_tile(capital_x, capital_y)
	}
	
	function get_map_mode_suffix(mode, province) {
		switch (mode) {
			case ProvinceMap.Mode.Political:
				return "/political"
			case ProvinceMap.Mode.Terrain:
				return "/terrain"
			case ProvinceMap.Mode.Cultural:
				return "/cultural"
			case ProvinceMap.Mode.Religious:
				return "/religious"
			case ProvinceMap.Mode.Technology:
				return "/technology"
			case ProvinceMap.Mode.TradeZone:
				return "/trade_zone"
			case ProvinceMap.Mode.Temple:
				return "/temple"
		}
		
		return ""
	}
	
	function update_civilian_unit_status_text(civilian_unit, show_text) {
		var text = ""
		
		if (civilian_unit !== null) {
			text = civilian_unit.type.name
			if (civilian_unit.working) {
				text += " (Working)"
			} else if (civilian_unit.moving) {
				text += " (Moving)"
			}
		}
		
		if (show_text) {
			status_text = text
		} else {
			if (status_text === text) {
				status_text = ""
			}
		}
	}
}
