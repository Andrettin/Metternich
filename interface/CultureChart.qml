import QtQuick
import QtQuick.Controls
import QtCharts

PopulationChart {
	id: chart
	
	Connections {
		target: chart.data_source
		ignoreUnknownSignals: true //as there may be no selected data source
		
		function onCulture_counts_changed() {
			chart.update_chart()
		}
	}

	function update_chart() {
		pie_series.clear()

		if (chart.population_data === null && chart.data_source === null) {
			return
		}

		var population_per_culture = chart.population_data ? chart.population_data : chart.data_source.culture_counts
		for (var i = 0; i < population_per_culture.length; i++) {
			var culture = population_per_culture[i].key
			var count = population_per_culture[i].value
			var pie_slice = pie_series.append(culture.name, count)
			pie_slice.color = culture.color
			pie_slice.borderColor = "black"
		}
	}
}
