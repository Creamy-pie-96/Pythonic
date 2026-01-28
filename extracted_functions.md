1. template <typename T> std::string to_str(const T &val)  // include/pythonic/pythonicVars.hpp
2. inline bool is_heap_type(TypeTag tag)  // include/pythonic/pythonicVars.hpp
3. template <typename T> const T &var_get()  // include/pythonic/pythonicVars.hpp
4. template <typename T> T *var_get_if()  // include/pythonic/pythonicVars.hpp
5. template <typename T> const T *var_get_if() noexcept  // include/pythonic/pythonicVars.hpp
6. void destroy_heap_data()  // include/pythonic/pythonicVars.hpp
7. void copy_heap_data(const var &other)  // include/pythonic/pythonicVars.hpp
8. void move_heap_data(var &&other)  // include/pythonic/pythonicVars.hpp
9. static int getTypeRank(TypeTag t)  // include/pythonic/pythonicVars.hpp
10. static bool isUnsignedTag(TypeTag t)  // include/pythonic/pythonicVars.hpp
11. static bool isSignedIntegerTag(TypeTag t)  // include/pythonic/pythonicVars.hpp
12. static bool isFloatingTag(TypeTag t)  // include/pythonic/pythonicVars.hpp
13. var addPromoted(const var &other)  // include/pythonic/pythonicVars.hpp
14. var subPromoted(const var &other)  // include/pythonic/pythonicVars.hpp
15. var mulPromoted(const var &other)  // include/pythonic/pythonicVars.hpp
16. var divPromoted(const var &other)  // include/pythonic/pythonicVars.hpp
17. var modPromoted(const var &other)  // include/pythonic/pythonicVars.hpp
18. ~var()  // include/pythonic/pythonicVars.hpp
19. bool isNone()  // include/pythonic/pythonicVars.hpp
20. template <typename T> bool is()  // include/pythonic/pythonicVars.hpp
21. bool isNumeric()  // include/pythonic/pythonicVars.hpp
22. bool isIntegral()  // include/pythonic/pythonicVars.hpp
23. bool is_list() noexcept  // include/pythonic/pythonicVars.hpp
24. bool is_dict() noexcept  // include/pythonic/pythonicVars.hpp
25. bool is_set() noexcept  // include/pythonic/pythonicVars.hpp
26. bool is_string() noexcept  // include/pythonic/pythonicVars.hpp
27. bool is_int() noexcept  // include/pythonic/pythonicVars.hpp
28. bool is_double() noexcept  // include/pythonic/pythonicVars.hpp
29. bool is_float() noexcept  // include/pythonic/pythonicVars.hpp
30. bool is_bool() noexcept  // include/pythonic/pythonicVars.hpp
31. bool is_none() noexcept  // include/pythonic/pythonicVars.hpp
32. bool is_ordered_dict() noexcept  // include/pythonic/pythonicVars.hpp
33. bool is_ordered_set() noexcept  // include/pythonic/pythonicVars.hpp
34. bool is_long() noexcept  // include/pythonic/pythonicVars.hpp
35. bool is_long_long() noexcept  // include/pythonic/pythonicVars.hpp
36. bool is_long_double() noexcept  // include/pythonic/pythonicVars.hpp
37. bool is_uint() noexcept  // include/pythonic/pythonicVars.hpp
38. bool is_ulong() noexcept  // include/pythonic/pythonicVars.hpp
39. bool is_ulong_long() noexcept  // include/pythonic/pythonicVars.hpp
40. bool is_graph() noexcept  // include/pythonic/pythonicVars.hpp
41. bool is_any_integral() noexcept  // include/pythonic/pythonicVars.hpp
42. bool is_any_floating() noexcept  // include/pythonic/pythonicVars.hpp
43. bool is_any_numeric() noexcept  // include/pythonic/pythonicVars.hpp
44. List &as_list_unchecked()  // include/pythonic/pythonicVars.hpp
45. const List &as_list_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
46. Dict &as_dict_unchecked()  // include/pythonic/pythonicVars.hpp
47. const Dict &as_dict_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
48. Set &as_set_unchecked()  // include/pythonic/pythonicVars.hpp
49. const Set &as_set_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
50. OrderedDict &as_ordered_dict_unchecked()  // include/pythonic/pythonicVars.hpp
51. const OrderedDict &as_ordered_dict_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
52. OrderedSet &as_ordered_set_unchecked()  // include/pythonic/pythonicVars.hpp
53. const OrderedSet &as_ordered_set_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
54. int &as_int_unchecked()  // include/pythonic/pythonicVars.hpp
55. int as_int_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
56. double &as_double_unchecked()  // include/pythonic/pythonicVars.hpp
57. double as_double_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
58. float &as_float_unchecked()  // include/pythonic/pythonicVars.hpp
59. float as_float_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
60. bool &as_bool_unchecked()  // include/pythonic/pythonicVars.hpp
61. bool as_bool_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
62. long &as_long_unchecked()  // include/pythonic/pythonicVars.hpp
63. long as_long_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
64. long long &as_long_long_unchecked()  // include/pythonic/pythonicVars.hpp
65. long long as_long_long_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
66. long double &as_long_double_unchecked()  // include/pythonic/pythonicVars.hpp
67. long double as_long_double_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
68. unsigned int &as_uint_unchecked()  // include/pythonic/pythonicVars.hpp
69. unsigned int as_uint_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
70. unsigned long &as_ulong_unchecked()  // include/pythonic/pythonicVars.hpp
71. unsigned long as_ulong_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
72. unsigned long long &as_ulong_long_unchecked()  // include/pythonic/pythonicVars.hpp
73. unsigned long long as_ulong_long_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
74. GraphPtr &as_graph_unchecked()  // include/pythonic/pythonicVars.hpp
75. const GraphPtr &as_graph_unchecked() noexcept  // include/pythonic/pythonicVars.hpp
76. List &as_list()  // include/pythonic/pythonicVars.hpp
77. const List &as_list()  // include/pythonic/pythonicVars.hpp
78. Dict &as_dict()  // include/pythonic/pythonicVars.hpp
79. const Dict &as_dict()  // include/pythonic/pythonicVars.hpp
80. Set &as_set()  // include/pythonic/pythonicVars.hpp
81. const Set &as_set()  // include/pythonic/pythonicVars.hpp
82. OrderedDict &as_ordered_dict()  // include/pythonic/pythonicVars.hpp
83. const OrderedDict &as_ordered_dict()  // include/pythonic/pythonicVars.hpp
84. OrderedSet &as_ordered_set()  // include/pythonic/pythonicVars.hpp
85. const OrderedSet &as_ordered_set()  // include/pythonic/pythonicVars.hpp
86. int as_int()  // include/pythonic/pythonicVars.hpp
87. double as_double()  // include/pythonic/pythonicVars.hpp
88. float as_float()  // include/pythonic/pythonicVars.hpp
89. bool as_bool()  // include/pythonic/pythonicVars.hpp
90. long as_long()  // include/pythonic/pythonicVars.hpp
91. long long as_long_long()  // include/pythonic/pythonicVars.hpp
92. long double as_long_double()  // include/pythonic/pythonicVars.hpp
93. unsigned int as_uint()  // include/pythonic/pythonicVars.hpp
94. unsigned long as_ulong()  // include/pythonic/pythonicVars.hpp
95. unsigned long long as_ulong_long()  // include/pythonic/pythonicVars.hpp
96. int toInt()  // include/pythonic/pythonicVars.hpp
97. unsigned int toUInt()  // include/pythonic/pythonicVars.hpp
98. long toLong()  // include/pythonic/pythonicVars.hpp
99. unsigned long toULong()  // include/pythonic/pythonicVars.hpp
100. long long toLongLong()  // include/pythonic/pythonicVars.hpp
101. unsigned long long toULongLong()  // include/pythonic/pythonicVars.hpp
102. float toFloat()  // include/pythonic/pythonicVars.hpp
103. double toDouble()  // include/pythonic/pythonicVars.hpp
104. long double toLongDouble()  // include/pythonic/pythonicVars.hpp
105. static TypeTag getPromotedType(TypeTag a, TypeTag b)  // include/pythonic/pythonicVars.hpp
106. const char *type_cstr() noexcept  // include/pythonic/pythonicVars.hpp
107. TypeTag type_tag() noexcept  // include/pythonic/pythonicVars.hpp
108. template <typename T> T &get()  // include/pythonic/pythonicVars.hpp
109. template <typename T> const T &get()  // include/pythonic/pythonicVars.hpp
110. operator bool()  // include/pythonic/pythonicVars.hpp
111. explicit operator int()  // include/pythonic/pythonicVars.hpp
112. explicit operator long long()  // include/pythonic/pythonicVars.hpp
113. explicit operator double()  // include/pythonic/pythonicVars.hpp
114. explicit operator float()  // include/pythonic/pythonicVars.hpp
115. explicit operator size_t()  // include/pythonic/pythonicVars.hpp
116. explicit operator long double()  // include/pythonic/pythonicVars.hpp
117. var len()  // include/pythonic/pythonicVars.hpp
118. void append(const var &v)  // include/pythonic/pythonicVars.hpp
119. template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, var>>> void append(T &&v)  // include/pythonic/pythonicVars.hpp
120. void add(const var &v)  // include/pythonic/pythonicVars.hpp
121. template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, var>>> void add(T &&v)  // include/pythonic/pythonicVars.hpp
122. void extend(const var &other)  // include/pythonic/pythonicVars.hpp
123. void update(const var &other)  // include/pythonic/pythonicVars.hpp
124. var contains(const var &v)  // include/pythonic/pythonicVars.hpp
125. var has(const var &v)  // include/pythonic/pythonicVars.hpp
126. void remove(const var &v)  // include/pythonic/pythonicVars.hpp
127. iterator begin()  // include/pythonic/pythonicVars.hpp
128. iterator end()  // include/pythonic/pythonicVars.hpp
129. const_iterator begin()  // include/pythonic/pythonicVars.hpp
130. const_iterator end()  // include/pythonic/pythonicVars.hpp
131. const_iterator cbegin()  // include/pythonic/pythonicVars.hpp
132. const_iterator cend()  // include/pythonic/pythonicVars.hpp
133. var items()  // include/pythonic/pythonicVars.hpp
134. var keys()  // include/pythonic/pythonicVars.hpp
135. var values()  // include/pythonic/pythonicVars.hpp
136. var slice(long long start = 0, long long end = LLONG_MAX, long long step = 1)  // include/pythonic/pythonicVars.hpp
137. var slice(const var &start_var, const var &end_var, const var &step_var = var(1))  // include/pythonic/pythonicVars.hpp
138. var operator()(const var &start_var, const var &end_var, const var &step_var = var(1))  // include/pythonic/pythonicVars.hpp
139. var upper()  // include/pythonic/pythonicVars.hpp
140. var lower()  // include/pythonic/pythonicVars.hpp
141. var strip()  // include/pythonic/pythonicVars.hpp
142. var lstrip()  // include/pythonic/pythonicVars.hpp
143. var rstrip()  // include/pythonic/pythonicVars.hpp
144. var replace(const var &old_str, const var &new_str)  // include/pythonic/pythonicVars.hpp
145. var find(const var &substr)  // include/pythonic/pythonicVars.hpp
146. var startswith(const var &prefix)  // include/pythonic/pythonicVars.hpp
147. var endswith(const var &suffix)  // include/pythonic/pythonicVars.hpp
148. var isdigit()  // include/pythonic/pythonicVars.hpp
149. var isalpha()  // include/pythonic/pythonicVars.hpp
150. var isalnum()  // include/pythonic/pythonicVars.hpp
151. var isspace()  // include/pythonic/pythonicVars.hpp
152. var capitalize()  // include/pythonic/pythonicVars.hpp
153. var sentence_case()  // include/pythonic/pythonicVars.hpp
154. var title()  // include/pythonic/pythonicVars.hpp
155. var count(const var &substr)  // include/pythonic/pythonicVars.hpp
156. var reverse()  // include/pythonic/pythonicVars.hpp
157. var split(const var &delim = var(" "))  // include/pythonic/pythonicVars.hpp
158. var join(const var &lst)  // include/pythonic/pythonicVars.hpp
159. var center(int width, const var &fillchar = var(" "))  // include/pythonic/pythonicVars.hpp
160. var zfill(int width)  // include/pythonic/pythonicVars.hpp
161. size_t hash()  // include/pythonic/pythonicVars.hpp
162. template <typename... Args> var list(Args &&...args)  // include/pythonic/pythonicVars.hpp
163. template <typename... Args> var set(Args &&...args)  // include/pythonic/pythonicVars.hpp
164. template <typename... Args> var ordered_set(Args &&...args)  // include/pythonic/pythonicVars.hpp
165. inline var list()  // include/pythonic/pythonicVars.hpp
166. inline var set()  // include/pythonic/pythonicVars.hpp
167. inline var dict()  // include/pythonic/pythonicVars.hpp
168. inline var ordered_set()  // include/pythonic/pythonicVars.hpp
169. inline var ordered_dict()  // include/pythonic/pythonicVars.hpp
170. size_t node_count()  // include/pythonic/pythonicVars.hpp
171. size_t edge_count()  // include/pythonic/pythonicVars.hpp
172. size_t size()  // include/pythonic/pythonicVars.hpp
173. bool is_connected()  // include/pythonic/pythonicVars.hpp
174. bool has_cycle()  // include/pythonic/pythonicVars.hpp
175. bool has_edge(size_t from, size_t to)  // include/pythonic/pythonicVars.hpp
176. size_t out_degree(size_t node)  // include/pythonic/pythonicVars.hpp
177. size_t in_degree(size_t node)  // include/pythonic/pythonicVars.hpp
178. size_t add_node()  // include/pythonic/pythonicVars.hpp
179. size_t add_node(const var &data)  // include/pythonic/pythonicVars.hpp
180. void remove_node(size_t node)  // include/pythonic/pythonicVars.hpp
181. bool remove_edge(size_t from, size_t to, bool remove_reverse = true)  // include/pythonic/pythonicVars.hpp
182. void set_edge_weight(size_t from, size_t to, double weight)  // include/pythonic/pythonicVars.hpp
183. void reserve_edges_per_node(size_t per_node)  // include/pythonic/pythonicVars.hpp
184. void set_node_data(size_t node, const var &data)  // include/pythonic/pythonicVars.hpp
185. var &get_node_data(size_t node)  // include/pythonic/pythonicVars.hpp
186. const var &get_node_data(size_t node)  // include/pythonic/pythonicVars.hpp
187. inline var graph(size_t n)  // include/pythonic/pythonicVars.hpp
188. inline size_t len(const var &v)  // include/pythonic/pythonicVars.hpp
189. operator var()  // include/pythonic/pythonicVars.hpp
190. template <typename T> inline bool isinstance(const var &v)  // include/pythonic/pythonicVars.hpp
191. inline TypeTag type_name_to_tag(const char *type_name)  // include/pythonic/pythonicVars.hpp
192. inline bool isinstance(const var &v, const char *type_name)  // include/pythonic/pythonicVars.hpp
193. inline var Bool(const var &v)  // include/pythonic/pythonicVars.hpp
194. inline var repr(const var &v)  // include/pythonic/pythonicVars.hpp
195. inline var Str(const var &v)  // include/pythonic/pythonicVars.hpp
196. inline var Int(const var &v)  // include/pythonic/pythonicVars.hpp
197. inline var Long(const var &v)  // include/pythonic/pythonicVars.hpp
198. inline var LongLong(const var &v)  // include/pythonic/pythonicVars.hpp
199. inline var UInt(const var &v)  // include/pythonic/pythonicVars.hpp
200. inline var ULong(const var &v)  // include/pythonic/pythonicVars.hpp
201. inline var ULongLong(const var &v)  // include/pythonic/pythonicVars.hpp
202. inline var Double(const var &v)  // include/pythonic/pythonicVars.hpp
203. inline var Float(const var &v)  // include/pythonic/pythonicVars.hpp
204. inline var LongDouble(const var &v)  // include/pythonic/pythonicVars.hpp
205. inline var String(const var &v)  // include/pythonic/pythonicVars.hpp
206. inline var abs(const var &v)  // include/pythonic/pythonicVars.hpp
207. inline var min(const var &a, const var &b)  // include/pythonic/pythonicVars.hpp
208. inline var min(const var &lst)  // include/pythonic/pythonicVars.hpp
209. inline var max(const var &a, const var &b)  // include/pythonic/pythonicVars.hpp
210. inline var max(const var &lst)  // include/pythonic/pythonicVars.hpp
211. inline var sum(const var &lst, const var &start = var(0))  // include/pythonic/pythonicVars.hpp
212. inline var reversed_var(const var &v)  // include/pythonic/pythonicVars.hpp
213. inline var all_var(const var &lst)  // include/pythonic/pythonicVars.hpp
214. inline var any_var(const var &lst)  // include/pythonic/pythonicVars.hpp
215. template <typename Func> inline var map(Func func, const var &lst)  // include/pythonic/pythonicVars.hpp
216. template <typename Func> inline var filter(Func predicate, const var &lst)  // include/pythonic/pythonicVars.hpp
217. template <typename Func> inline var reduce(Func func, const var &lst, const var &initial)  // include/pythonic/pythonicVars.hpp
218. template <typename Func> inline var reduce(Func func, const var &lst)  // include/pythonic/pythonicVars.hpp
219. inline var input(const var &prompt = var(""))  // include/pythonic/pythonicVars.hpp
220. inline var input(const char *prompt)  // include/pythonic/pythonicVars.hpp
221. template <typename Tuple> auto get(const Tuple &t, size_t index) -> std::enable_if_t<(std::tuple_size_v<std::decay_t<Tuple>> > 0), var>  // include/pythonic/pythonicVars.hpp
222. template <typename Tuple, size_t... Is> var get_impl(const Tuple &t, size_t index, std::index_sequence<Is...>)  // include/pythonic/pythonicVars.hpp
223. template <typename... Ts> var tuple_to_list(const std::tuple<Ts...> &t)  // include/pythonic/pythonicVars.hpp
224. template <typename Tuple> var unpack(const Tuple &t)  // include/pythonic/pythonicVars.hpp
225. #endif #define let(name)  // include/pythonic/pythonicVars.hpp