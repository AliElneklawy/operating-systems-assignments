#include "caltrain.h"

void
station_init(struct station *station)
{
	station -> train_stat = 0; //initially, the train is not in the station
	station -> free_seats_stat = 0; //initially, there are 0 seats available
	station -> free_seats_num = 0;
	station -> pssngrs_stat = 0;  //initially, 0 passengers are waiting
	station -> pssngrs_num = 0;

	pthread_cond_init(&station->free_seats_avail, NULL);
	pthread_cond_init(&station->train_arrived, NULL);
	pthread_cond_init(&station->pssngr_boarded, NULL);

	pthread_mutex_init(&station->mutex, NULL);
}

void
station_load_train(struct station *station, int count) //count: number of free seats
{
	pthread_mutex_lock(&station->mutex); //make sure that only one train is in the station

	station -> free_seats_num = count;
	if(station->free_seats_num > 0) station -> free_seats_stat = 1;
	station -> train_stat = 1;  //the train arrived

	 while ((station->free_seats_stat == 1) && (station->pssngrs_stat == 1)){  //free seats are available and passengers are waiting
		pthread_cond_signal(&station->train_arrived);
		pthread_cond_signal(&station->free_seats_avail); //notify passengers that there are free seats
		pthread_cond_wait(&station->pssngr_boarded, &station->mutex); //wait for the passenger to board
	}

	station -> train_stat = 0;  // the train left
	station -> free_seats_stat = 0;

	pthread_mutex_unlock(&station->mutex);
}

void
station_wait_for_train(struct station *station)
{
	pthread_mutex_lock(&station->mutex); //enforce mutual exclusion

	station->pssngrs_stat = 1; // there are passengers in the station
	station->pssngrs_num++; // one more passenger arrived

	while((station->train_stat == 0) || (station->free_seats_stat == 0)){ //wait for the train or free seats

		if(station->train_stat == 0)
			pthread_cond_wait(&station->train_arrived, &station->mutex);

		if(station->free_seats_stat == 0)
			pthread_cond_wait(&station->free_seats_avail, &station->mutex);
	}

	pthread_mutex_unlock(&station->mutex);
}

void
station_on_board(struct station *station)
{
	pthread_mutex_lock(&station->mutex);

	station->free_seats_num--; // a passenger took over one chair
	station->pssngrs_num--;

	if(station->pssngrs_num <= 0)
		station->pssngrs_stat = 0;

	if(station->free_seats_num <= 0)
		station->free_seats_stat = 0;

	pthread_cond_signal(&station->pssngr_boarded);  //passenger is on the train

	pthread_mutex_unlock(&station->mutex);
}
