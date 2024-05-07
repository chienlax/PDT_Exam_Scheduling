#include "solver.h"
#include <iostream>

using namespace std;

Solver::Solver(Instance* instance) : instance(instance) {
	E = instance->E;
	S = instance->S;
	R = instance->R;
	T = instance->T;
	C = instance->C;
	capacity = instance->capacity;
	numberOfStudentInExam = instance->numberOfStudentInExam;
	examOfStudent = instance->examOfStudent;
	exam_and_infor = instance->exam_and_infor;
	student_and_infor = instance->student_and_infor;
	decoding_exam = instance->decoding_exam;
	decoding_student = instance->decoding_student;

	void run();
}

void Solver::run() {
	cout << "\n-----------------------\n\nStart of solver\n\n";

	IloEnv env;
	IloModel model(env);

	// Declare Variable:
	IloArray<IloArray<IloNumVarArray>> x_jrt(env, E); // 1 nếu môn j được thi tại phòng r và time slot t
	IloArray<IloNumVarArray> x_jt(env, E); // 1 nếu môn j được thi ở time slot t
	IloArray<IloArray<IloArray<IloNumVarArray>>> w_ijrt(env, S); // sinh viên i thi môn j ở phòng r time slot t
	IloArray<IloArray<IloNumVarArray>> w_ijt(env, S); // sinh viên i thi môn j ở time slot t
	IloArray<IloNumVarArray> w_it(env, S); // sinh viên i thi ở time slot t
	IloArray<IloArray<IloNumVarArray>> wi_pq(env, S); // sinh viên i thi ở trong các time slot p và q
	IloArray<IloArray<IloNumVarArray>> zj_pq(env, E); // môn j được tổ chức liên tiếp từ time-slot p đến time slot q
	IloArray<IloArray<IloArray<IloNumVarArray>>> xij_rt(env, E); // hai môn i và j cùng thi tại phòng (r, t)

	for (int j = 0; j < E; j++) {
		x_jrt[j] = IloArray<IloNumVarArray>(env, R);
		for (int r = 0; r < R; r++) {
			x_jrt[j][r] = IloNumVarArray(env, T);
			for (int t = 0; t < T; t++) {
				x_jrt[j][r][t] = IloNumVar(env, 0, 1, ILOINT);
			}
		}
	}

	for (int j = 0; j < E; j++) {
		x_jt[j] = IloNumVarArray(env, T);
		for (int t = 0; t < T; t++) {
			x_jt[j][t] = IloNumVar(env, 0, 1, ILOINT);
		}
	}

	for (int i = 0; i < S; i++) {
		w_ijrt[i] = IloArray<IloArray<IloNumVarArray>>(env, E);
		for (int j = 0; j < E; j++) {
			w_ijrt[i][j] = IloArray<IloNumVarArray>(env, R);
			for (int r = 0; r < R; r++) {
				w_ijrt[i][j][r] = IloNumVarArray(env, T);
				for (int t = 0; t < T; t++) {
					w_ijrt[i][j][r][t] = IloNumVar(env, 0, 1, ILOINT);
				}
			}
		}
	}

	for (int i = 0; i < S; i++) {
		w_ijt[i] = IloArray<IloNumVarArray>(env, E);
		for (int j = 0; j < E; j++) {
			w_ijt[i][j] = IloNumVarArray(env, T);
			for (int t = 0; t < T; t++) {
				w_ijt[i][j][t] = IloNumVar(env, 0, 1, ILOINT);
			}
		}
	}

	for (int i = 0; i < S; i++) {
		w_it[i] = IloNumVarArray(env, T);
		for (int t = 0; t < T; t++) {
			w_it[i][t] = IloNumVar(env, 0, 1, ILOINT);
		}
	}

	for (int i = 0; i < S; i++) {
		wi_pq[i] = IloArray<IloNumVarArray>(env, T);
		for (int p = 0; p < T; p++) {
			wi_pq[i][p] = IloNumVarArray(env, T);
			for (int q = 0; q < T; q++) {
				wi_pq[i][p][q] = IloNumVar(env, 0, 1, ILOINT);
			}
		}
	}

	for (int j = 0; j < E; j++) {
		zj_pq[j] = IloArray<IloNumVarArray>(env, T);
		for (int p = 0; p < T; p++) {
			zj_pq[j][p] = IloNumVarArray(env, T);
			for (int q = 0; q < T; q++) {
				zj_pq[j][p][q] = IloNumVar(env, 0, 1, ILOINT);
			}
		}
	}

	for (int j1 = 0; j1 < E; j1++) {
		xij_rt[j1] = IloArray<IloArray<IloNumVarArray>>(env, E);
		for (int j2 = 0; j2 < E; j2++) {
			if (j1 != j2) {
				xij_rt[j1][j2] = IloArray<IloNumVarArray>(env, R);
				for (int r = 0; r < R; r++) {
					xij_rt[j1][j2][r] = IloNumVarArray(env, T);
					for (int t = 0; t < T; t++) {
						xij_rt[j1][j2][r][t] = IloNumVar(env, 0, 1, ILOINT);
					}
				}
			}
		}
	}

	// Constraints:
	// (1) sinh viên i tham gia không quá một môn ở thời điểm t với mọi cặp (i, t)
	for (int i = 0; i < S; i++) {
		for (int t = 0; t < T; t++) {
			IloExpr expr1(env);
			for (int j = 0; j < E; j++) {
				expr1 += w_ijt[i][j][t];
			}
			model.add(expr1 <= 1);
		}
	}

	// (2) số sinh viên tham gia phòng thi r thỏa mãn điều kiện giới hạn 
	// số sinh viên của phòng tại thời điểm t bất kỳ
	for (int r = 0; r < R; r++) {
		for (int t = 0; t < T; t++) {
			IloExpr expr2(env);
			for (int i = 0; i < S; i++) {
				for (int j = 0; j < E; j++) {
					expr2 += w_ijrt[i][j][r][t];
				}
			}
			model.add(expr2 <= capacity[r]);
		}
	}

	// (3) sinh viên i phải thi môn j ở phòng r và ở tại thời điểm t
	// nào đó với mọi cặp (i, t) mà sinh viên i thi môn t
	// Với mọi sinh viên, tại 1 thời điểm, chỉ thi 1 môn tại 1 phòng nhất định

	for (int i = 0; i < S; i++) {
		for (auto j : examOfStudent[i]) { // j chỉ chạy trên những exam của sinh viên i
			IloExpr expr3(env);
			for (int r = 0; r < R; r++) {
				for (int t = 0; t < T; t++) {
					expr3 += w_ijrt[i][j][r][t];
				}
			}
			model.add(expr3 == 1);
		}
	}

	// (4) sinh viên chỉ thi tại phòng mà nó được mở; và môn thi
	// được tổ chức ở timeslot đó
	for (int i = 0; i < S; i++) {
		for (int j = 0; j < E; j++) {
			for (int r = 0; r < R; r++) {
				for (int t = 0; t < T; t++) {
					model.add(w_ijrt[i][j][r][t] <= x_jrt[j][r][t]);
				}
			}
		}
	}

	for (int j = 0; j < E; j++) {
		for (int r = 0; r < R; r++) {
			for (int t = 0; t < T; t++) {
				model.add(x_jrt[j][r][t] <= x_jt[j][t]);
			}
		}
	}

	// (5) phải có ít nhất 1 phòng được sử dụng để thi môn j ở thời điểm t
	for (int j = 0; j < E; j++) {
		for (int t = 0; t < T; t++) {
			IloExpr expr4(env);
			for (int r = 0; r < R; r++) {
				expr4 += x_jrt[j][r][t];
			}
			model.add(expr4 >= x_jt[j][t]);
		}
	}

	// (6) Hội constraint chia phòng
	for (int j1 = 0; j1 < E; j1++) {
		for (int j2 = j1 + 1; j2 < E; j2++) {
			for (int r = 0; r < R; r++) {
				for (int t = 0; t < T; t++) {
					model.add(xij_rt[j1][j2][r][t] <= x_jrt[j1][r][t]);
				}
			}
		}
	}

	for (int j1 = 0; j1 < E; j1++) {
		for (int j2 = j1 + 1; j2 < E; j2++) {
			for (int r = 0; r < R; r++) {
				for (int t = 0; t < T; t++) {
					model.add(xij_rt[j1][j2][r][t] <= x_jrt[j2][r][t]);
				}
			}
		}
	}

	for (int j1 = 0; j1 < E; j1++) {
		for (int j2 = j1 + 1; j2 < E; j2++) {
			for (int r = 0; r < R; r++) {
				for (int t = 0; t < T; t++) {
					model.add(xij_rt[j1][j2][r][t] >= x_jrt[j1][r][t] + x_jrt[j2][r][t] - 1);
				}
			}
		}
	}

	// (7) phải có ít nhất một sinh viên i thi môn j ở thời điểm t
	for (int j = 0; j < E; j++) {
		for (int r = 0; r < R; r++) {
			for (int t = 0; t < T; t++) {
				IloExpr expr5(env);
				for (int i = 0; i < S; i++) {
					expr5 += w_ijrt[i][j][r][t];
				}
				model.add(expr5 >= x_jrt[j][r][t]);
			}
		}
	}

	// (8) số phòng mở phục vụ môn học j phải thỏa mãn số sinh
	// viên cho môn này
	for (int j = 0; j < E; j++) {
		IloExpr expr6(env);
		for (int r = 0; r < R; r++) {
			for (int t = 0; t < T; t++) {
				expr6 += (x_jrt[j][r][t] * capacity[r]);
			}
		}
		model.add(expr6 >= numberOfStudentInExam[j]);
	}

	// (9) sinh viên i sẽ thi tất cả các môn ở trong time slot từ
	// p..q với p, q nào đó(nghĩa là sẽ thi trong một vài slot ở trong này)
	for (int i = 0; i < S; i++) {
		IloExpr expr7(env);
		for (int p = 0; p < T; p++) {
			for (int q = 0; q < T; q++) {
				if (p <= q)
					expr7 += wi_pq[i][p][q];
			}
		}
		model.add(expr7 == 1);
	}

	// (10) nếu khoảng [p,q] là khoảng thời gian mà sinh viên i thi thì p là 
	// thời điểm thi sớm nhất và q là thời điểm thi muộn nhất của sinh viên i
	for (int i = 0; i < S; i++) {
		for (int p = 0; p < T; p++) {
			for (int q = 0; q < T; q++) {
				if (p <= q) {
					model.add(w_it[i][p] == w_it[i][q]);
					model.add(w_it[i][q] == wi_pq[i][p][q]);
				}
			}
		}
	}

	// (11) nếu khoảng [p,q] là khoảng thời gian mà sinh viên i thi thì p là 
	// thời điểm thi sớm nhất và q là thời điểm thi muộn nhất của sinh viên i
	for (int i = 0; i < S; i++) {
		for (int t = 0; t < T; t++) {
			for (int p = 0; p < T; p++) {
				for (int q = 0; q < T; q++) {
					if (p <= q && (t < p || t > q)) {
						model.add(w_it[i][t] + wi_pq[i][p][q] <= 1);
					}
				}
			}
		}
	}

	// (12) nếu [p, q] là khoảng được chọn thì sinh viên ko thi sau thời điểm p.
	for (int i = 0; i < S; i++) {
		for (int p = 0; p < T; p++) {
			for (int q = p; q < T; q++) {
				IloExpr expr8(env);
				for (int t = 0; t < p; t++) {
					expr8 += w_it[i][t];
					model.add(expr8 <= (p - 1) * (1 - wi_pq[i][p][q]));
				}
			}
		}
	}

	// (13)  nếu [p, q] là khoảng được chọn thì sinh viên ko thi trước thời điểm p.
	for (int i = 0; i < S; i++) {
		for (int p = 0; p < T; p++) {
			for (int q = 0; q < T; q++) {
				if (p <= q) {
					IloExpr expr9(env);
					for (int t = q + 1; t < T; t++) {
						expr9 += w_it[i][t];
					}
					model.add(expr9 <= (T - q) * (1 - wi_pq[i][p][q]));
				}

			}
		}
	}

	// (14) môn j sẽ được thi trên 1 bộ slot liên tiếp
	for (int j = 0; j < E; j++) {
		IloExpr expr10(env);
		for (int p = 0; p < T; p++) {
			for (int q = 0; q < T; q++) {
				if (p <= q)
					expr10 += zj_pq[j][p][q];
			}
		}
		model.add(expr10 == 1);
	}

	// (15)  nếu đoạn p..q được tổ chức để thi môn j(wj_pq = 1) 
	// thì bất cứ slot nào cũng phải có môn j thi (hay xjt = 1)
	for (int j = 0; j < E; j++) {
		for (int p = 0; p < T; p++) {
			for (int q = 0; q < T; q++) {
				if (p <= q) {
					for (int t = 0; t < T; t++) {
						if (p <= t <= q) {
							model.add(x_jt[j][t] >= zj_pq[j][p][q]);
						}
					}
				}
			}
		}
	}

	// Objective function:
	IloExpr obj1(env);
	for (int j = 0; j < E; j++) {
		for (int p = 0; p < T; p++) {
			for (int q = 0; q < T; q++) {
				if (p <= q) {
					obj1 += zj_pq[j][p][q] * (q - p);
				}
			}
		}
	}
	obj1 *= 9;

	IloExpr obj2(env);
	for (int i = 0; i < S; i++) {
		for (int p = 0; p < T; p++) {
			for (int q = 0; q < T; q++) {
				if (p <= q) {
					if (q <= 6 || p >= 7)
						obj2 += wi_pq[i][p][q] * (q - p);
				}
			}
		}
	}
	obj2 *= 4;

	IloExpr obj3(env);
	for (int i = 0; i < S; i++) {
		for (int p = 0; p < T; p++) {
			for (int q = p; q < T; q++) {
				if (p <= q) {
					if (p <= 6 || q >= 6)
						obj3 += wi_pq[i][p][q] * (q - p);
				}
			}
		}
	}
	obj3 *= 5;

	IloExpr obj4(env);
	for (int j = 0; j < E; j++) {
		for (int r = 0; r < R; r++) {
			for (int t = 0; t < T; t++) {
				obj4 += x_jrt[j][r][t];
			}
		}
	}
	obj4 *= 7;

	IloExpr obj5(env);
	for (int j1 = 0; j1 < E; j1++) {
		for (int j2 = 0; j2 < E; j2++) {
			if (j1 != j2) {
				for (int r = 0; r < R; r++) {
					for (int t = 0; t < T; t++) {
						obj5 += xij_rt[j1][j2][r][t];
					}
				}
			}
		}
	}
	obj5 *= 8;

	model.add(IloMinimize(env, obj1 + obj2 + obj3 + obj4 + obj5));

	cout << "\nCplex starts here\n";

	IloCplex cplex(model);
	if (!cplex.solve()) {
		cerr << "Failed to solve the problem" << endl;
		throw(-1);
	}

	cout << "Solved\n";

	double objValue = cplex.getObjValue();
	std::cout << "Objective Value: " << objValue << std::endl;

	cout << "Solution status: " << cplex.getStatus() << endl;
}