import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Item {
	id: map_block
	implicitWidth: metternich.defines.map_block_size.width * metternich.defines.province_map_tile_scale * scale_factor
	implicitHeight: metternich.defines.map_block_size.height * metternich.defines.province_map_tile_scale * scale_factor
	clip: true
	
	readonly property int scaled_map_block_start_x: map_block_start_x * metternich.defines.province_map_tile_scale * scale_factor
	readonly property int scaled_map_block_start_y: map_block_start_y * metternich.defines.province_map_tile_scale * scale_factor
	readonly property int map_block_index: index
	
	Item {
		id: unscaled_province_map
		width: metternich.defines.map_block_size.width * metternich.defines.province_map_tile_scale
		height: metternich.defines.map_block_size.height * metternich.defines.province_map_tile_scale
		clip: true
		layer.enabled: true
		
		Repeater {
			model: provinces
			
			Shape {
				id: province_shape
				x: -map_block_start_x * metternich.defines.province_map_tile_scale
				y: -map_block_start_y * metternich.defines.province_map_tile_scale
				visible: province_polygon_path.length > 0
				
				readonly property var province: model.modelData
				readonly property var province_polygon_path: province ? province.map_data.polygon_path : ""
				readonly property var selected: selected_province === province && (selected_garrison === false || province_map.show_site_mode === ProvinceMap.SiteMode.ShowLocations)
				readonly property var interactive: selected_civilian_unit !== null && !selected_civilian_unit.busy && selected_civilian_unit_interactive_provinces.includes(province)
				property int change_count: 0
				
				ShapePath {
					strokeColor: fillColor
					fillColor: selected ? metternich.defines.selected_country_color : (interactive ? "darkGreen" : get_map_mode_color(province_map.mode, province, change_count)) //the change count is there to force a re-evaluation of the binding if a relevant property has changed
					startX: 0
					startY: 0
					
					PathSvg {
						path: province_polygon_path
					}
				}
				
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
							case ProvinceMap.Mode.CulturalSociety:
								if (map_mode_identifier !== "cultural_society") {
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
		width: scaled_province_map.width
		height: scaled_province_map.height
		hoverEnabled: true
		
		onReleased: function (mouse) {
			metternich.defines.click_sound.play()
			
			var province = metternich.map.get_tile_province(Qt.point(map_block_start_x + Math.floor(mouse.x / metternich.defines.province_map_tile_scale / scale_factor), Math.floor(map_block_start_y + mouse.y / metternich.defines.province_map_tile_scale / scale_factor)))
			
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
			var province = metternich.map.get_tile_province(Qt.point(map_block_start_x + Math.floor(mouse.x / metternich.defines.province_map_tile_scale / scale_factor), Math.floor(map_block_start_y + mouse.y / metternich.defines.province_map_tile_scale / scale_factor)))
			
			if (province !== null) {
				var text = get_province_status_text(province)
				status_text = text
			} else {
				status_text = ""
			}
		}
	}
	
	Repeater {
		model: routes
		
		Shape {
			id: route_shape
			x: -scaled_map_block_start_x
			y: -scaled_map_block_start_y
			visible: route && route.game_data.active && route.type !== null && (province_map.mode === ProvinceMap.Mode.TradeZone || route.type.output_commodity !== metternich.defines.wealth_commodity) && (province_map.mode === ProvinceMap.Mode.Temple || route.type.output_commodity !== metternich.defines.piety_commodity)
			
			readonly property var route: model.modelData
			readonly property var route_line_path: route ? route.game_data.get_line_path() : ""
			
			ShapePath {
				strokeWidth: 2 * scale_factor
				strokeColor: metternich.defines.route_color
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
		model: provinces
		
		Item {
			id: province_label_area
			x: -scaled_map_block_start_x + Math.max(Math.floor(text_rect.x * metternich.defines.province_map_tile_scale * scale_factor) + Math.floor((text_rect_width - width) / 2), Math.floor((province_label.contentWidth - width) / 2 + 1 * scale_factor))
			y: -scaled_map_block_start_y + Math.floor(text_rect.y * metternich.defines.province_map_tile_scale * scale_factor) + Math.max(0, Math.floor((text_rect_height - height) / 2))
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
									hovered_icon_map_block_index = map_block_index
								} else {
									if (hovered_icon_map_block_index === map_block_index) {
										if (status_text === text) {
											status_text = ""
										}
										
										hovered_icon_map_block_index = -1
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
										hovered_icon_map_block_index = map_block_index
									} else {
										if (hovered_icon_map_block_index === map_block_index) {
											if (status_text === text) {
												status_text = ""
											}
											
											hovered_icon_map_block_index = -1
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
										hovered_icon_map_block_index = map_block_index
									} else {
										if (hovered_icon_map_block_index === map_block_index) {
											if (status_text === text) {
												status_text = ""
											}
											
											if (hovered_site === site) {
												hovered_site = null
											}
											
											hovered_icon_map_block_index = -1
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
		model: sites
		
		Item {
			id: site_icon_area
			x: -scaled_map_block_start_x + (site ? Math.min(Math.max(site.game_data.tile_pos.x * metternich.defines.province_map_tile_scale * scale_factor - Math.floor(width / 2), 0), province_map.contentWidth - width) : 0)
			y: -scaled_map_block_start_y + (site ? Math.min(Math.max(site.game_data.tile_pos.y * metternich.defines.province_map_tile_scale * scale_factor - Math.floor(height / 2), 0), province_map.contentHeight - height) : 0)
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
						hovered_icon_map_block_index = map_block_index
					} else {
						if (hovered_icon_map_block_index === map_block_index) {
							if (status_text === text) {
								status_text = ""
							}
							
							if (hovered_site === site) {
								hovered_site = null
							}
							
							hovered_icon_map_block_index = -1
						}
					}
				}
			}
		}
	}
}
