using System.Collections;
using UnityEngine;
using Firebase;
using Firebase.Database;
using UnityEngine.UI;

public class VuforiaFirebaseManager : MonoBehaviour
{
    // Firebase variables
    private DatabaseReference databaseReference;

    // UI elements to display the data (make sure these are in your AR scene)
    public Text humidityText;
    public Text plantNameText;
    public Text soilHumidityText;
    public Text temperatureText;
    public Text waterTimeText;
    public Text waterTimeCmpText;

    private bool firebaseInitialized = false;

    // Refresh interval
    private float refreshInterval = 15f;

    void Start()
    {
        // Initialize Firebase asynchronously
        FirebaseApp.CheckAndFixDependenciesAsync().ContinueWith(task => {
            var dependencyStatus = task.Result;
            if (dependencyStatus == DependencyStatus.Available)
            {
                // Create a database reference
                databaseReference = FirebaseDatabase.DefaultInstance.RootReference;
                Debug.Log("Firebase initialized successfully!");
                firebaseInitialized = true;
            }
            else
            {
                Debug.LogError(System.String.Format(
                  "Could not resolve all Firebase dependencies: {0}", dependencyStatus));
            }
        });

        // Start refreshing data immediately
        StartCoroutine(RefreshData()); 
    }

    // Coroutine to refresh data every 15 seconds
    IEnumerator RefreshData()
    {
        while (true)
        {
            if (firebaseInitialized) 
            {
                LoadData();
            }
            yield return new WaitForSeconds(refreshInterval);
        }
    }

    // Load data from Firebase
    void LoadData()
    {
        databaseReference.Child("plant1").GetValueAsync().ContinueWith(task => {
            if (task.IsFaulted)
            {
                Debug.LogError("Failed to load data: " + task.Exception);
            }
            else if (task.IsCompleted)
            {
                DataSnapshot snapshot = task.Result;
                // Log the raw JSON data to the console
                Debug.Log("Raw Data: " + snapshot.GetRawJsonValue());

                // Extract the data from the snapshot
                string humidity = snapshot.Child("humidity").Value.ToString();
                string plantName = snapshot.Child("plant_name").Value.ToString();
                string soilHumidity = snapshot.Child("soil_humidity").Value.ToString();
                string temperature = snapshot.Child("temperature").Value.ToString();
                string waterTime = snapshot.Child("water_time").Value.ToString();
                string waterTimeCmp = snapshot.Child("water_time_cmp").Value.ToString();

                // Update the UI elements on the main thread
                UnityMainThreadDispatcher.Instance().Enqueue(() => {
                    if (humidityText != null)
                    {
                        humidityText.text = humidity;
                    }
                    else
                    {
                        Debug.LogError("humidityText is not assigned!");
                    }

                    if (plantNameText != null)
                    {
                        plantNameText.text = plantName;
                    }
                    else
                    {
                        Debug.LogError("plantNameText is not assigned!");
                    }

                    if (soilHumidityText != null)
                    {
                        soilHumidityText.text = soilHumidity;
                    }
                    else
                    {
                        Debug.LogError("soilHumidityText is not assigned!");
                    }

                    if (temperature != null)
                    {
                        temperatureText.text = temperature;
                    }
                    else
                    {
                        Debug.LogError("temperatureText is not assigned!");
                    }

                    if (waterTimeText != null)
                    {
                        waterTimeText.text = waterTime;
                    }
                    else
                    {
                        Debug.LogError("waterTimeText is not assigned!");
                    }

                    if (waterTimeCmpText != null)
                    {
                        waterTimeCmpText.text = waterTimeCmp;
                    }
                    else
                    {
                        Debug.LogError("waterTimeCmpText is not assigned!");
                    }
                });
            }
        });
    }
}
