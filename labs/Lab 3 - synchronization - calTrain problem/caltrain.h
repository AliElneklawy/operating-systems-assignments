#include <pthread.h>

struct station {

	int train_stat; //1: the train is in the station. 0: it is not
	pthread_cond_t train_arrived;

	int free_seats_stat; //1: seats are available. 0: not available
	int free_seats_num; //number of free seats
	pthread_cond_t free_seats_avail;  //wait on this condition when there are no free seats

	int pssngrs_stat; //1: passengers are waiting. 0: no passengers in the station
	int pssngrs_num; //number of passengers
	pthread_cond_t pssngr_boarded;

	pthread_mutex_t mutex;
};

void station_init(struct station *station);

/*stay inside it until the train is fully loaded or there
are no passengers waiting. once loaded, signal it to leave
using pthread_cond_signal*/
void station_load_train(struct station *station, int count);

/*stay inside it until the train is in the station and
there are enough seats for the passengers to sit down*/
void station_wait_for_train(struct station *station);

/*called when a passenger is seated. lets the train know
that the passenger has sit down*/
void station_on_board(struct station *station);
