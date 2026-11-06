#pragma once

#include <string>
#include <vector>

namespace uilo {

/*
	Fits a single-line UTF-8 label to a measured width without modifying its source text.
	Only complete codepoints are retained. An area narrower than the suffix draws nothing.

	measure: Returns the rendered width of a UTF-8 string in the caller's font and scale.
*/
template <class Measure>
std::string ellipsizeText(const std::string& text, float width, Measure measure) {
	if (width <= 0) return {};
	if (measure(text) <= width) return text;
	const std::string suffix = "...";
	if (measure(suffix) > width) return {};
	std::vector<size_t> boundaries{0};
	for (size_t i = 1; i < text.size(); ++i) {
		if ((static_cast<unsigned char>(text[i]) & 0xc0) != 0x80) boundaries.push_back(i);
	}
	size_t lo = 0, hi = boundaries.size();
	while (lo + 1 < hi) {
		const size_t mid = lo + (hi - lo) / 2;
		if (measure(text.substr(0, boundaries[mid]) + suffix) <= width) lo = mid;
		else hi = mid;
	}
	return text.substr(0, boundaries[lo]) + suffix;
}

} // namespace uilo
