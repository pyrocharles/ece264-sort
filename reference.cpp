/*
 * reference.cpp - holds an old sort implementation so I may see if I'm
 *                 improving
 */

#include <cstdint>
#include <cstring>

#include "lookup_tables.hpp"

// minimum characters needed to check uniqueness
namespace Min_Chars {
constexpr const size_t last_name = 9;
constexpr const size_t first_name = 9;
}

inline uint32_t ssn_to_int(const string& ssn) {
	constexpr const size_t length = 11;
	char to_convert[length];
	strncpy(to_convert, ssn.c_str(), length);
	return 100000000*to_convert[ 0]
	      + 10000000*to_convert[ 1]
	       + 1000000*to_convert[ 2]
	        + 100000*to_convert[ 4]
	         + 10000*to_convert[ 5]
	          + 1000*to_convert[ 7]
	           + 100*to_convert[ 8]
	            + 10*to_convert[ 9]
	             + 1*to_convert[10];
}

/* enters the given number of characters from the given string into the given
   integer, most-significant-place-justified. only for CAPITALIZED A-Z
   strings... fine for this */
template<typename Integer>
inline Integer string_to_int(const string& str,
                             const size_t chars) {
	const size_t string_length = str.length();
	const size_t first_pass = string_length < chars ? string_length : chars;
	char to_convert[9] { '@', '@', '@', '@', '@', '@', '@', '@', '@' };
	// TODO: move that 9 somewhere else

	strncpy(to_convert, str.c_str(), first_pass);

	constexpr const static Integer offset =
		 282429536481L*'@'
		+ 10460353203L*'@'
		  + 387420489L*'@'
		   + 14348907L*'@'
		     + 531441L*'@'
		      + 19683L*'@'
		        + 729L*'@'
		         + 27L*'@'
		          + 1L*'@';

	return 282429536481L*to_convert[0]
	      + 10460353203L*to_convert[1]
	        + 387420489L*to_convert[2]
	         + 14348907L*to_convert[3]
	           + 531441L*to_convert[4]
	            + 19683L*to_convert[5]
	              + 729L*to_convert[6]
	               + 27L*to_convert[7]
	                + 1L*to_convert[8] - offset;
}

inline uint16_t last_name_to_int(const string& name) {
	return last_name_table[
		string_to_int<uint64_t>(name, Min_Chars::last_name)
			% last_name_table_size
	];
}

inline uint16_t first_name_to_int(const string& name) {
	return first_name_table[
		string_to_int<uint64_t>(name, Min_Chars::first_name)
			% first_name_table_size
	];
}

struct Data_Ref {
	uint32_t ssn;
	uint16_t last_name;
	uint16_t first_name;
	Data* data;

	inline void initialize(Data* data, bool do_names);
};

inline void Data_Ref::initialize(Data* data, bool do_names) {
	this->data = data;
	ssn = ssn_to_int(data->ssn);

	if (do_names) {
		last_name = last_name_to_int(data->lastName);
		first_name = first_name_to_int(data->firstName);
	} else {
		last_name = 0;
		first_name = 0;
	}
}

template<size_t Bin_Count, size_t Bin_Size>
struct Bin_Array {
	static constexpr const size_t max_size = Bin_Count;

	struct Bin {
		static constexpr const size_t max_size = Bin_Size;

		size_t size;
		Data_Ref contents[Bin_Size];

		inline Bin() : size(0) { }
		inline Data_Ref operator[](size_t index) { return contents[index]; }
		inline void push(Data_Ref& next) { contents[size++] = next; }
	};

	Bin bins[Bin_Count];

	inline Bin_Array() { }
	inline Bin& operator[](size_t index) { return bins[index]; }
};

constexpr const size_t maximum_items = 1'010'000; // one percent over a million
Data_Ref entries[maximum_items];

constexpr const uint_fast8_t radix_bits = 9;
constexpr const size_t radix_base = 1 << radix_bits;
constexpr const size_t radix_mask = radix_base - 1;
constexpr const uint_fast8_t max_radix_shift_first = 0;
constexpr const uint_fast8_t max_radix_shift_last = 0;

constexpr const size_t bin_size = (maximum_items / radix_base) << 4;
Bin_Array<radix_base, bin_size> bin_array;

void radix_sort_ssns(const size_t count) {
	constexpr const size_t bits = 8;
	constexpr const size_t base = 1 << bits;
	constexpr const size_t mask = base - 1;
	constexpr const uint_fast8_t max_shift = 24;

	size_t shift = 0;
	while (true) {
		for (size_t i = 0; i < count; ++i) {
			const uint32_t ssn = entries[i].ssn;
			const size_t bin = (ssn >> shift) & mask;
			bin_array[bin].push(entries[i]);
		}

		// TODO: add a bin iterator that makes this easy
		size_t entry = 0;
		for (size_t bin = 0; bin < base; ++bin) {
			for (size_t i = 0; i < bin_array[bin].size; ++i) {
				entries[entry++] = bin_array[bin][i];
			}
			bin_array[bin].size = 0;
		}

		if (shift == max_shift) break;
		shift += bits;
	}
}

void radix_sort_first_names(const size_t count) {
	size_t shift = 0;

	while (true) {
		for (size_t i = 0; i < count; ++i) {
			const uint16_t first_name = entries[i].first_name;
			const size_t bin = (first_name >> shift) & radix_mask;
			bin_array[bin].push(entries[i]);
		}

		// TODO: add a bin iterator that makes this easy
		size_t entry = 0;
		for (size_t bin = 0; bin < radix_base; ++bin) {
			for (size_t i = 0; i < bin_array[bin].size; ++i) {
				entries[entry++] = bin_array[bin][i];
			}
			bin_array[bin].size = 0;
		}

		if (shift == max_radix_shift_first) break;
		shift += radix_bits;
	}
}

void radix_sort_last_names(const size_t count) {
	size_t shift = 0;

	while (true) {
		for (size_t i = 0; i < count; ++i) {
			const uint16_t last_name = entries[i].last_name;
			const size_t bin = (last_name >> shift) & radix_mask;
			bin_array[bin].push(entries[i]);
		}

		// TODO: add a bin iterator that makes this easy
		size_t entry = 0;
		for (size_t bin = 0; bin < radix_base; ++bin) {
			for (size_t i = 0; i < bin_array[bin].size; ++i) {
				entries[entry++] = bin_array[bin][i];
			}
			bin_array[bin].size = 0;
		}

		if (shift == max_radix_shift_last) break;
		shift += radix_bits;
	}
}

inline bool ordered(const Data_Ref& first, const Data_Ref& second) {
	const auto last_name_1 = first.last_name;
	const auto last_name_2 = second.last_name;
	const auto first_name_1 = first.first_name;
	const auto first_name_2 = second.first_name;
	const auto ssn_1 = first.ssn;
	const auto ssn_2 = second.ssn;

	if (last_name_1 < last_name_2) {
		return true;
	} else if (last_name_1 == last_name_2) {
		if (first_name_1 < first_name_2) {
			return true;
		} else if (first_name_1 == first_name_2) {
			if (ssn_1 <= ssn_2) {
				return true;
			}
		}
	}

	return false;
}

void insertion_sort(const size_t count) {
	for (size_t i = 0; i < count; ++i) {
		Data_Ref elem = entries[i];

		size_t j = i;
		for (; j > 0; --j) {
			if (ordered(elem, entries[j - 1])) {
				// less than the current element, so should go earlier;
				// move the section forward
				entries[j] = entries[j - 1];
			} else {
				// found the spot!
				break;
			}
		}

		entries[j] = elem;
	}
}

void sortDataList(list<Data *> &l) {
	const size_t length = l.size();
	auto head = l.begin();

	const bool likely_set_3_or_4 =
		((*head++)->lastName.compare((*head)->lastName) == 0)
		&& ((*head++)->lastName.compare((*head)->lastName) == 0);
	const bool likely_set_4 =
		likely_set_3_or_4
		&& (l.front()->lastName.compare(l.back()->lastName) == 0);

	const bool init_names = !likely_set_4;

	size_t index = 0;
	for (auto iter = l.begin(); iter != l.end(); ++iter)
		entries[index++].initialize(*iter, init_names);

	if (likely_set_4) {
		radix_sort_ssns(SORT_LENGTH);
	} else {
		if (!likely_set_3_or_4) {
			radix_sort_first_names(SORT_LENGTH);
			radix_sort_last_names(SORT_LENGTH);
		}
		insertion_sort(SORT_LENGTH);
	}

	index = 0;
	for (auto iter = l.begin(); iter != l.end(); ++iter)
		*iter = entries[index++].data;
}

/*
 * Copyright (C) 2022, C. R. Van West
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
