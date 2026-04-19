module.exports = [
  {
    type: 'heading',
    defaultValue: 'Settings'
  },
  {
    type: 'heading',
    defaultValue: 'Favorite Stations'
  },
  {
    type: 'text',
    id: 'station-picker',
    defaultValue: ''
  },
  {
    type: 'input',
    messageKey: 'FAVORITES',
    label: 'Favorites',
    defaultValue: '[]'
  },
  {
    type: 'toggle',
    messageKey: 'DISRUPTIONS',
    label: 'Show disruption notices',
    defaultValue: true
  },
  {
    type: 'select',
    messageKey: 'LANGUAGE',
    label: 'Language',
    defaultValue: 'auto',
    options: [
      { label: 'Auto (system)', value: 'auto' },
      { label: 'English', value: 'en' },
      { label: 'Deutsch', value: 'de' },
      { label: 'Français', value: 'fr' },
      { label: 'Italiano', value: 'it' }
    ]
  },
  {
    type: 'submit',
    defaultValue: 'Save'
  }
];
